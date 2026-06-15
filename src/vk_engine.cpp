//
// Created by kyleb on 6/1/2026.
//

#include "vk_engine.h"
#include "vk_initializers.h"
#include "vk_types.h"
#include "vk_images.h"
#include "vk_descriptors.h"
#include "vk_pipelines.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "VkBootstrap.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <chrono>
#include <thread>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

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

    SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN;

    window_ = SDL_CreateWindow(
        "Vulkan Engine",
        static_cast<int>(windowExtent_.width),
        static_cast<int>(windowExtent_.height),
        window_flags);

    init_vulkan();
    init_swapchain();
    init_commands();
    init_sync_structures();
    init_descriptors();
    init_pipelines();
    init_imgui();

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

    // set swapchain image layout to Attachment Optimal so we can draw it
    vkutil::transition_image(cmd, swapchainImages_[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    draw_imgui(cmd, swapchainImageViews_[swapchainImageIndex]);

    // Transition the swapchain image to a presentable state
    vkutil::transition_image(cmd, dstImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

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

    ComputeEffect& effect = computeEffects[currentComputeEffect];

    // Bind the gradient drawing compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.pipeline);
    // Bind the descriptor set containing the draw image to the compute pipeline
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, gradientPipelineLayout_, 0, 1, &drawImageDescriptors_, 0, nullptr);

    vkCmdPushConstants(cmd, gradientPipelineLayout_, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.data);
    // Execute the compute pipeline dispatch.
    // Group count is determined by image dimensions / workgroup size
    vkCmdDispatch(cmd, std::ceil(drawExtent_.width / 16.0), std::ceil(drawExtent_.height / 16.0), 1);

}

void VulkanEngine::draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView) {
    VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(targetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderInfo = vkinit::rendering_info(swapchainExtent_, &colorAttachment);

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
}

void VulkanEngine::run() {
    assert(loadedEngine != nullptr && "VulkanEngine has not been loaded!");

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

            ImGui_ImplSDL3_ProcessEvent(&e);
        }

        if (pauseRendering_) {
            // Throttle speed
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // ImGui Menu Logic

        if (ImGui::Begin("background")) {
            ComputeEffect& selected = computeEffects[currentComputeEffect];
            ImGui::Text("Selected Effect: ", selected.name);
            ImGui::SliderInt("Effect Index: ", &currentComputeEffect, 0, computeEffects.size() - 1);
            ImGui::InputFloat4("data1", (float*)&selected.data.data1);
            ImGui::InputFloat4("data2", (float*)&selected.data.data2);
            ImGui::InputFloat4("data3", (float*)&selected.data.data3);
            ImGui::InputFloat4("data4", (float*)&selected.data.data4);
        }
        ImGui::End();

        // End ImGui Menu Logic

        ImGui::Render();

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

    // ImGui
    VK_CHECK(vkCreateCommandPool(device_, &commandPoolInfo, nullptr, &immCommandPool_));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(immCommandPool_, 1);

    VK_CHECK(vkAllocateCommandBuffers(device_, &cmdAllocInfo, &immCommandBuffer_));

    mainDeletionQueue_.push_function([=]() {
        vkDestroyCommandPool(device_, immCommandPool_, nullptr);
    });
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

    // ImGui
    VK_CHECK(vkCreateFence(device_, &fenceCreateInfo, nullptr, &immFence_));
    mainDeletionQueue_.push_function([=](){
        vkDestroyFence(device_, immFence_, nullptr);
    });
}

void VulkanEngine::init_descriptors() {
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .ratio = 1
        }
    };

    globalDescriptorAllocator.init_pool(device_, 10, sizes);

    {
        DescriptorLayoutBuilder builder;
        builder.add_binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        drawImageDescriptorLayout_ = builder.build(device_, VK_SHADER_STAGE_COMPUTE_BIT, nullptr, 0);
    }

    drawImageDescriptors_ = globalDescriptorAllocator.allocate(device_, drawImageDescriptorLayout_);

    VkDescriptorImageInfo imgInfo = {};
    imgInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imgInfo.imageView = drawImage_.imageView;

    VkWriteDescriptorSet drawImageWrite = {};
    drawImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    drawImageWrite.pNext = nullptr;

    drawImageWrite.dstBinding = 0;
    drawImageWrite.dstSet = drawImageDescriptors_;
    drawImageWrite.descriptorCount = 1;
    drawImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    drawImageWrite.pImageInfo = &imgInfo;

    vkUpdateDescriptorSets(device_, 1, &drawImageWrite, 0, nullptr);
    mainDeletionQueue_.push_function([&]()
    {
        globalDescriptorAllocator.destroy_pool(device_);
        vkDestroyDescriptorSetLayout(device_, drawImageDescriptorLayout_, nullptr);
    });
}

void VulkanEngine::init_pipelines() {
    init_background_pipelines();
}

void VulkanEngine::init_background_pipelines() {
    VkPipelineLayoutCreateInfo computeLayout = {};
    computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    computeLayout.pNext = nullptr;
    computeLayout.pSetLayouts = &drawImageDescriptorLayout_;
    computeLayout.setLayoutCount = 1;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ComputePushConstants);
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    computeLayout.pPushConstantRanges = &pushConstantRange;
    computeLayout.pushConstantRangeCount = 1;

    VK_CHECK(vkCreatePipelineLayout(device_, &computeLayout, nullptr, &gradientPipelineLayout_));

    VkShaderModule gradientGridShader;
    if (!vkutil::load_shader_module("shaders/gradient_color.comp.spv", device_, &gradientGridShader)) {
        fmt::print("Error when building the compute shader\n");
    }

    VkShaderModule gradientShader;
    if (!vkutil::load_shader_module("shaders/gradient.comp.spv", device_, &gradientShader)) {
        fmt::print("Error when building the compute shader\n");
    }

    VkPipelineShaderStageCreateInfo stageInfo = {};
    stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stageInfo.pNext = nullptr;
    stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    stageInfo.module = gradientGridShader;
    stageInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCreateInfo = {};
    computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCreateInfo.pNext = nullptr;
    computePipelineCreateInfo.layout = gradientPipelineLayout_;
    computePipelineCreateInfo.stage = stageInfo;

    ComputeEffect gradientGrid;
    gradientGrid.layout = gradientPipelineLayout_;
    gradientGrid.name = "gradientGrid";
    gradientGrid.data = {};

    gradientGrid.data.data1 = glm::vec4(1, 0, 0, 1);
    gradientGrid.data.data2 = glm::vec4(0, 0, 1, 1);

    VK_CHECK(vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradientGrid.pipeline));

    computePipelineCreateInfo.stage.module = gradientShader;
    ComputeEffect gradient;
    gradient.layout = gradientPipelineLayout_;
    gradient.name = "gradient";
    gradient.data = {};

    gradient.data.data1 = glm::vec4(1, 0, 0, 1);
    gradient.data.data2 = glm::vec4(0, 0, 1, 1);

    VK_CHECK(vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.pipeline));

    computeEffects.push_back(gradient);
    computeEffects.push_back(gradientGrid);

    vkDestroyShaderModule(device_, gradientGridShader, nullptr);
    vkDestroyShaderModule(device_, gradientShader, nullptr);

    mainDeletionQueue_.push_function([&]()
    {
        vkDestroyPipelineLayout(device_, gradientPipelineLayout_, nullptr);
        vkDestroyPipeline(device_, gradientGrid.pipeline, nullptr);
        vkDestroyPipeline(device_, gradient.pipeline, nullptr);
    });
}

void VulkanEngine::init_imgui() {
    // Descriptor pool for ImGui
    VkDescriptorPoolSize poolSizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = uint32_t(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;

    VkDescriptorPool imguiPool;
    VK_CHECK(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &imguiPool));

    // Initialize ImGui Library
    ImGui::CreateContext();

    ImGui_ImplSDL3_InitForVulkan(window_);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = instance_;
    initInfo.PhysicalDevice = chosenGpu_;
    initInfo.Device = device_;
    initInfo.Queue = graphicsQueue_;
    initInfo.DescriptorPool = imguiPool;
    initInfo.MinImageCount = 3;
    initInfo.ImageCount = 3;
    initInfo.UseDynamicRendering = true;

    // Dynamic rendering parameters for ImGui
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &swapchainImageFormat_,
    };

    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);

    mainDeletionQueue_.push_function([=](){
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(device_, imguiPool, nullptr);
    });
}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function) {
    VK_CHECK(vkResetFences(device_, 1, &immFence_));
    VK_CHECK(vkResetCommandBuffer(immCommandBuffer_, 0));

    VkCommandBuffer cmd = immCommandBuffer_;

    VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd));

    VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
    VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

    VK_CHECK(vkQueueSubmit2(graphicsQueue_, 1, &submit, immFence_));

    VK_CHECK(vkWaitForFences(device_, 1, &immFence_, true, 9999999999));
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