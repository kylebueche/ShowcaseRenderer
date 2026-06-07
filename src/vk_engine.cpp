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

#include <chrono>
#include <thread>

constexpr bool bUseValidationLayers = false;

VulkanEngine* loadedEngine = nullptr;

VulkanEngine& VulkanEngine::Get() { return *loadedEngine; }

void VulkanEngine::init() {
    // Elegant way to ensure a singleton
    assert(loadedEngine == nullptr && "Duplicate VulkanEngine initialization not allowed!");
    loadedEngine = this;

    // Initialize SDL
    SDL_Init(SDL_INIT_VIDEO);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN);

    _window = SDL_CreateWindow(
        "Vulkan Engine",
        _windowExtent.width,
        _windowExtent.height,
        window_flags);

    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();

    _isInitialized = true;
}

void VulkanEngine::cleanup() {
    if (_isInitialized) {
        // Make sure the gpu has stopped doing work
        vkDeviceWaitIdle(_device);

        for (int i = 0; i < FRAME_OVERLAP; i++) {
            vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
            vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
            vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
            vkDestroySemaphore(_device, _frames[i]._swapchainSemaphore, nullptr);
        }

        destroy_swapchain();
        vkDestroySurfaceKHR(_instance, _surface, nullptr);
        vkDestroyDevice(_device, nullptr);
        vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
        vkDestroyInstance(_instance, nullptr);
        SDL_DestroyWindow(_window);
    }

    loadedEngine = nullptr;
}

void VulkanEngine::draw() {
    // Wait until the GPU has finished rendering the last frame.
    VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
    VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));
    uint32_t swapchainImageIndex;
    VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._swapchainSemaphore, nullptr, &swapchainImageIndex));

    VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

    // Commands have finished executing, we can safely reset the buffer.
    VK_CHECK(vkResetCommandBuffer(cmd, 0));

    // Begin command buffer recording. We will use this only once, so we let vulkan know that.
    // The flag might help optimize by telling drivers our intended usage :D
    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    // Start the command buffer recording
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    // Transition the framebuffers.
    VkImage img = _swapchainImages[swapchainImageIndex];
    // Put the swapchain image into a writeable state.
    vkutil::transition_image(cmd, img, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    VkClearColorValue clearValue = {};
    glm::vec3 clearColor = glm::vec3(abs(sin(_frameNumber / 120.0f)));
    clearValue = { clearColor.r, clearColor.g, clearColor.b, 1.0 };

    VkImageSubresourceRange clearRange = vkinit::image_subresource_range(VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd, img, VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    // Transition the swapchain into presentable layout to display to the screen
    vkutil::transition_image(cmd, img, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submitInfo, get_current_frame()._renderFence));

    // Actually present the image to the screen.
    // Wait on the _renderSemaphre for this, because we need all drawing commands to be finished first.
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext = nullptr;
    presentInfo.pSwapchains = &_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pImageIndices = &swapchainImageIndex;
    VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

    _frameNumber++;
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
                stop_rendering = true;
            } else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                stop_rendering = false;
            }
        }

        if (stop_rendering) {
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

    _instance = vkb_inst.instance;
    _debugMessenger = vkb_inst.debug_messenger;

    SDL_Vulkan_CreateSurface(_window, _instance, NULL, &_surface);

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
        .set_surface(_surface)
        .select()
        .value();

    // Create the Final Vulkan Device
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    // Get the VkDevice handle
    _device = vkbDevice.device;
    _chosenGPU = physicalDevice.physical_device;

    // End Vulkan Device Initialization

    // Get a Graphics Queue
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

}

void VulkanEngine::init_swapchain() {
    create_swapchain(_windowExtent.width, _windowExtent.height);
}

void VulkanEngine::init_commands() {
    // Create a command pool for commands submitted to the graphics queue.
    // Allow the pool to reset individual command buffers
    VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

    for (int i = 0; i < FRAME_OVERLAP; i++) {
        VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

        VkCommandBufferAllocateInfo commandAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(_device, &commandAllocInfo, &_frames[i]._mainCommandBuffer));
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
        VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._swapchainSemaphore));
        VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));
    }
}

void VulkanEngine::create_swapchain(uint32_t width, uint32_t height) {
    vkb::SwapchainBuilder swapchainBuilder { _chosenGPU, _device, _surface };

    _swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::Swapchain vkbSwapchain = swapchainBuilder
        //.use_default_format_selection()
        .set_desired_format(VkSurfaceFormatKHR{ .format = _swapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
        // Use vsync present mode
        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
        .set_desired_extent(width, height)
        .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
        .build()
        .value();

    _swapchainExtent = vkbSwapchain.extent;
    // Store swapchain and its related images
    _swapchain = vkbSwapchain.swapchain;
    _swapchainImages = vkbSwapchain.get_images().value();
    _swapchainImageViews = vkbSwapchain.get_image_views().value();

}

void VulkanEngine::destroy_swapchain() {
    vkDestroySwapchainKHR(_device, _swapchain, nullptr);

    // Destroy swapchain resources
    for (int i = 0; i < _swapchainImageViews.size(); i++) {
        vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
    }
}