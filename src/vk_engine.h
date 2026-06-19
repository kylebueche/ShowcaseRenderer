// Copyright (c) 2026 Kyle Bueche.
// Author: Kyle Bueche.
// This project is licensed under the MIT Licence - see LICENSE.txt.
// No warranty implied.

#ifndef SHOWCASERENDERER_VK_ENGINE_H_
#define SHOWCASERENDERER_VK_ENGINE_H_

#include "vk_types.h"
#include "vk_initializers.h"
#include "vk_descriptors.h"

#include <vk_mem_alloc.h>

struct DeletionQueue
{
    std::deque<std::function<void()>> deletionQueue;

    void push_function(std::function<void()>&& function) {
        deletionQueue.push_back(function);
    }

    void flush() {
        // Reverse iterate the deletion queue
        for (auto func = deletionQueue.rbegin(); func != deletionQueue.rend(); func++) {
            (*func)(); // Call functions
        }
        deletionQueue.clear();
    }
};

struct FrameData
{
    VkCommandPool commandPool;
    VkCommandBuffer mainCommandBuffer;
    // These names confuse me:
    VkSemaphore swapchainSemaphore; // Rendering waits on the swapchain to swap
    VkSemaphore renderSemaphore; // Swapping the swapchain waits on rendering to finish
    VkFence renderFence; // Wait for draw commands to finish before issuing new draw commands
    DeletionQueue deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;

struct ComputePushConstants
{
    glm::vec4 data1;
    glm::vec4 data2;
    glm::vec4 data3;
    glm::vec4 data4;
};

struct ComputeEffect
{
    const char* name;

    VkPipeline pipeline;
    VkPipelineLayout layout;

    ComputePushConstants data;
};

class VulkanEngine
{
public:

    static VulkanEngine& Get();

    void init();
    void cleanup();
    void run(); // Main loop

    DescriptorAllocator globalDescriptorAllocator;
    std::vector<ComputeEffect> computeEffects;
    int currentComputeEffect = 0;

private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();
    void init_descriptors();
    void init_pipelines();
    void init_background_pipelines();
    void init_imgui();

    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();

    FrameData& get_current_frame();

    void draw(); // Draw skeleton
    void draw_background(VkCommandBuffer cmd); // Draw commands
    // ImGui
    void draw_imgui(VkCommandBuffer cmd, VkImageView targetImageView);
    void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

    // -- Engine State --
    bool isInitialized = false;
    int frameNumber = 0;
    bool pauseRendering = false;

    // -- SDL / Windowing --
    struct SDL_Window* window = nullptr;
    VkExtent2D windowExtent = { .width=1600, .height=900 };
    VkSurfaceKHR surface = VK_NULL_HANDLE; // Vulkan Window Surface

    // -- Vulkan --
    // Main
    VkInstance instance = VK_NULL_HANDLE; // Vulkan Library Handle
    VkDevice device = VK_NULL_HANDLE; // Logical Vulkan Device for Commands
    VkPhysicalDevice chosenGpu = VK_NULL_HANDLE; // Physical GPU Selected
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkFormat swapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent = { .width=0, .height=0 };
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    FrameData frames[FRAME_OVERLAP] = {};
    AllocatedImage drawImage = {};
    VkExtent2D drawExtent = {.width=0, .height=0};

    // Memory Management
    DeletionQueue mainDeletionQueue = {};
    VmaAllocator allocator = VK_NULL_HANDLE;
    VkDescriptorSet drawImageDescriptors = VK_NULL_HANDLE;
    VkDescriptorSetLayout drawImageDescriptorLayout = VK_NULL_HANDLE;
    VkPipeline gradientPipeline = VK_NULL_HANDLE;
    VkPipelineLayout gradientPipelineLayout = VK_NULL_HANDLE;

    // -- ImGui --
    VkFence immFence = VK_NULL_HANDLE;
    VkCommandBuffer immCommandBuffer = VK_NULL_HANDLE;
    VkCommandPool immCommandPool = VK_NULL_HANDLE;

};

#endif //SHOWCASERENDERER_VK_ENGINE_H_