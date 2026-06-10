//
// Created by kyleb on 6/1/2026.
//

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include "vk_images.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "VkBootstrap.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <chrono>
#include <thread>

constexpr bool bUseValidationLayers = false;

static VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() {
    assert(loadedEngine != nullptr && "VulkanEngine has not been loaded!");
    return *loadedEngine;
}

FrameData& VulkanEngine::get_current_frame() {
    return frames_[frameNumber_ % FRAME_OVERLAP];
}

void VulkanEngine::init() {
    // Elegant way to ensure a singleton
    assert(loadedEngine == nullptr && "Duplicate VulkanEngine initialization not allowed!");
    loadedEngine = this;

    // Initialize SDL
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    window_ = SDL_CreateWindow(
        "Vulkan Engine",
        windowExtent_.width,
        windowExtent_.height,
        window_flags);

    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();

    isInitialized_ = true;
}

void VulkanEngine::cleanup() {
    if (isInitialized_) {
        // Make sure the gpu has stopped doing work
        vkDeviceWaitIdle(device_);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(device_, frames_[i]._commandPool, nullptr);
            vkDestroyFence(device_, frames_[i]._renderFence, nullptr);
            vkDestroySemaphore(device_, frames_[i]._renderSemaphore, nullptr);
            vkDestroySemaphore(device_, frames_[i]._swapchainSemaphore, nullptr);
            frames_[i].deletionQueue.flush();
        }
        mainDeletionQueue_.flush();

        destroy_swapchain();
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        vkDestroyDevice(device_, nullptr);
        vkb::destroy_debug_utils_messenger(instance_, debugMessenger_);
        vkDestroyInstance(instance_, nullptr);
        SDL_DestroyWindow(window_);
    }

    loadedEngine = nullptr;
}

void VulkanEngine::draw() {
    // Wait until the GPU has finished rendering the last frame.
    VK_CHECK(vkWaitForFences(device_, 1, &get_current_frame()._renderFence, true, 1000000000));
    get_current_frame().deletionQueue.flush();
    VK_CHECK(vkResetFences(device_, 1, &get_current_frame()._renderFence));
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(device_, swapchain_, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    // Commands have finished executing, we can safely reset the buffer.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // Begin command buffer recording. We will use this only once, so we let vulkan know that.
    // The flag might help optimize by telling drivers our intended usage :D
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    drawExtent_.width = windowExtent_.width;
    drawExtent_.height = windowExtent_.height;

    // Start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));


    // Transition main draw image into general, drawable layout.
    vkutil::transition_image(cmd, drawImage_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // Actual draw commands
    draw_background(cmd);

    // Copy the draw image to the current swapchain image
    VkImage dstImage = swapchainImages_[swapchainImageIndex];
    vkutil::transition_image(cmd, drawImage_.image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    vkutil::transition_image(cmd, dstImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    vkutil::copy_image_to_image(cmd, drawImage_.image, dstImage, drawExtent_, swapchainExtent_);

    // Transition the swapchain image to a presentable state
    vkutil::transition_image(cmd, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // Finalize Command Buffer
    VK_CHECK(vkEndCommandBuffer(cmd));

    // Prepare the submission for the queue
    // Wait on the presenting semaphore
    // Signal to the render semaphore to signal that rendering has finished

    VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
    VkSemaphoreSubmitInfo waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, get_current_frame()._swapchainSemaphore);
    VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, get_current_frame()._renderSemaphore);

    VkSubmitInfo2 submitInfo = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);

    // Submit command buffer to the queue and execute it.
    // _renderFence will now block until the graphics commands finish execution.
    VK_CHECK(vkQueueSubmit2(graphicsQueue_, 1, &submitInfo, get_current_frame()._renderFence));

    // Actually present the image to the screen.
    // Wait on the _renderSemaphre for this, because we need all drawing commands to be finished first.
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;
    VK_CHECK(vkQueuePresentKHR(graphicsQueue_, &presentInfo));

    frameNumber_++;
}

void VulkanEngine::draw_background(VkCommandBuffer cmd) {
    VkClearColorValue clearValue = {};
    float flash = abs(sin(frameNumber_ / 120.0f));
    clearValue = { 0.0, 0.0, flash, 1.0 };
    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, drawImage_.image, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
}

void VulkanEngine::run() {
    SDL_Event e;
    bool bQuit = false;

    // Main Loop
    while (!bQuit) {
        while (SDL_PollEvent(&e) != 0) {
            // Close the window on alt-f4 or x button
            if (e.type == SDL_EVENT_QUIT) {
                bQuit = true;
            } else if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                pauseRendering_ = true;
            } else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                pauseRendering_ = false;
            }
        }

        if (pauseRendering_) {
            // Throttle speed
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        draw();
    }
}

void VulkanEngine::init_vulkan() {
    vkb::InstanceBuilder builder;

    auto inst_ret = builder.set_app_name("Example Vulkan Application")
        .request_validation_layers(bUseValidationLayers)
        .use_default_debug_messenger()
        .require_api_version(1, 3, 0)
        .build();

    vkb::Instance vkb_inst = inst_ret.value();

    instance_ = vkb_inst.instance;
    debugMessenger_ = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(window_, instance_, NULL, &surface_);

    // Vulkan 1.3 features
    VkPhysicalDeviceVulkan13Features features13{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    // Vulkan 1.2 features
    VkPhysicalDeviceVulkan12Features features12{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    // Use vkbootstrap to select a GPU.
    // Check for a gpu that can write to the SDL surface
    // and supports Vulkan 1.3 with the desired features

    vkb::PhysicalDeviceSelector selector{ vkb_inst };
    vkb::PhysicalDevice physicalDevice = selector
        .set_minimum_version(1, 3)
        .set_required_features_13(features13)
        .set_required_features_12(features12)
        .set_surface(surface_)
        .select()
        .value();

    // Create the Final Vulkan Device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle
    device_ = vkbDevice.device;
    chosenGpu_ = physicalDevice.physical_device;

    // End Vulkan Device Initialization

    // Get a Graphics Queue
    graphicsQueue_ = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    graphicsQueueFamily_ = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

    // Initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = chosenGpu_;
    allocatorInfo.device = device_;
    allocatorInfo.instance = instance_;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    vmaCreateAllocator(&allocatorInfo, &allocator_);
    mainDeletionQueue_.push_function([&]()
    {
        vmaDestroyAllocator(allocator_);
    });
}

void VulkanEngine::init_swapchain() {
    create_swapchain(windowExtent_.width, windowExtent_.height);
    VkExtent3D drawImageExtent = {
        windowExtent_.width,
        windowExtent_.height,
        1
    };
    drawImage_.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    drawImage_.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimg_info = vkinit::image_create_info(drawImage_.imageFormat, drawImageUsages, drawImageExtent);

    // Allocate it from GPU local memory
    VmaAllocationCreateInfo rimg_alloc_info = {};
    rimg_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimg_alloc_info.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // Allocate and create the image
    vmaCreateImage(allocator_, &rimg_info, &rimg_alloc_info, &drawImage_.image, &drawImage_.allocation, nullptr);

    // Build an image-view for the draw image to use for rendering
    VkImageViewCreateInfo rview_info = vkinit::image_view_create_info(drawImage_.imageFormat, drawImage_.image, VK_IMAGE_ASPECT_COLOR_BIT);
    VK_CHECK(vkCreateImageView(device_, &rview_info, nullptr, &drawImage_.imageView));

    mainDeletionQueue_.push_function([this]()
    {
        vkDestroyImageView(device_, drawImage_.imageView, nullptr);
        vmaDestroyImage(allocator_, drawImage_.image, drawImage_.allocation);
    });

}

void VulkanEngine::init_commands() {
    // Create a command pool for commands submitted to the graphics queue.
    // Allow the pool to reset individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(graphicsQueueFamily_, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(device_, &commandPoolInfo, nullptr, &frames_[i]._commandPool));

        VkCommandBufferAllocateInfo commandAllocInfo = vkinit::command_buffer_allocate_info(frames_[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device_, &commandAllocInfo, &frames_[i]._mainCommandBuffer));
    }
}

void VulkanEngine::init_sync_structures() {
    // Create synchronization structures
    // One fence to control when the GPU has finished rendering the frame,
    // and two semaphores to synchronize rendering with the swapchain.
    // We want the fence to start signalled so we can wait on it for the first frame.
    VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info(0);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateFence(device_, &fenceCreateInfo, nullptr, &frames_[i]._renderFence));
        VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &frames_[i]._swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(device_, &semaphoreCreateInfo, nullptr, &frames_[i]._renderSemaphore));
    }
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchainBuilder { chosenGpu_, device_, surface_ };

    swapchainImageFormat_ = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = swapchainImageFormat_, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        // Use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    swapchainExtent_ = vkbSwapchain.extent;
    // Store swapchain and its related images
    swapchain_ = vkbSwapchain.swapchain;
    swapchainImages_ = vkbSwapchain.get_images().value();
    swapchainImageViews_ = vkbSwapchain.get_image_views().value();

}

void VulkanEngine::destroy_swapchain() {
    vkDestroySwapchainKHR(device_, swapchain_, nullptr);

    // Destroy swapchain resources
    for (int i = 0; i < swapchainImageViews_.size(); i++) {
        vkDestroyImageView(device_, swapchainImageViews_[i], nullptr);
    }
}