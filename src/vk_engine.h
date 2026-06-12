//
// Created by kyleb on 5/31/2026.
//

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
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    // These names confuse me:
    VkSemaphore _swapchainSemaphore; // Rendering waits on the swapchain to swap
    VkSemaphore _renderSemaphore; // Swapping the swapchain waits on rendering to finish
    VkFence _renderFence; // Wait for draw commands to finish before issuing new draw commands
    DeletionQueue deletionQueue;
};

constexpr unsigned int FRAME_OVERLAP = 2;


class VulkanEngine
{
public:

    static VulkanEngine& Get();

    void init();
    void cleanup();
    void run(); // Main loop

    DescriptorAllocator globalDescriptorAllocator;

private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();
    void init_descriptors();
    void init_pipelines();
    void init_background_pipelines();

    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();

    FrameData& get_current_frame();

    void draw(); // Draw skeleton
    void draw_background(VkCommandBuffer cmd); // Draw commands

    // -- Engine State --
    bool isInitialized_ = false;
    int frameNumber_ = 0;
    bool pauseRendering_ = false;

    // -- SDL / Windowing --
    struct SDL_Window* window_ = nullptr;
    VkExtent2D windowExtent_ = { .width=1600, .height=900 };
    VkSurfaceKHR surface_ = VK_NULL_HANDLE; // Vulkan Window Surface

    // -- Vulkan --
    // Main
    VkInstance instance_ = VK_NULL_HANDLE; // Vulkan Library Handle
    VkDevice device_ = VK_NULL_HANDLE; // Logical Vulkan Device for Commands
    VkPhysicalDevice chosenGpu_ = VK_NULL_HANDLE; // Physical GPU Selected
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily_ = 0;

    // Debug
    VkDebugUtilsMessengerEXT debugMessenger_ = VK_NULL_HANDLE;

    // Swapchain
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapchainImageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent_ = { .width=0, .height=0 };
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainImageViews_;
    FrameData frames_[FRAME_OVERLAP] = {};
    AllocatedImage drawImage_ = {};
    VkExtent2D drawExtent_ = {.width=0, .height=0};

    // Memory Management
    DeletionQueue mainDeletionQueue_ = {};
    VmaAllocator allocator_ = VK_NULL_HANDLE;
    VkDescriptorSet drawImageDescriptors_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout drawImageDescriptorLayout_ = VK_NULL_HANDLE;
    VkPipeline gradientPipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout gradientPipelineLayout_ = VK_NULL_HANDLE;
};


#endif //SHOWCASERENDERER_VK_ENGINE_H_