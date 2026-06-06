//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_ENGINE_H
#define SHOWCASERENDERER_VK_ENGINE_H

#include "vk_types.h"
#include "vk_initializers.h"

struct FrameData
{
    VkCommandPool _commandPool;
    VkCommandBuffer _mainCommandBuffer;
    // These names confuse me:
    VkSemaphore _swapchainSemaphore; // Rendering waits on the swapchain to swap
    VkSemaphore _renderSemaphore; // Swapping the swapchain waits on rendering to finish
    VkFence _renderFence; // Wait for draw commands to finish before issuing new draw commands
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine
{
public:

    bool _isInitialized{ false };
    int _frameNumber {0};
    bool stop_rendering{ false };
    VkExtent2D _windowExtent{ 1600, 900 };

    struct SDL_Window* _window{ nullptr };
    static VulkanEngine& Get();

    void init();
    void cleanup();
    void draw(); // Draw loop
    void run(); // Main loop

    VkInstance _instance; // Vulkan Library Handle
    VkDebugUtilsMessengerEXT _debugMessenger; // Vulkan Debug Output Handle
    VkPhysicalDevice _chosenGPU; // Physical GPU Selected
    VkDevice _device; // Logical Vulkan Device for Commands
    VkSurfaceKHR _surface; // Vulkan Window Surface

    VkSwapchainKHR _swapchain;
    VkFormat _swapchainImageFormat;

    std::vector<VkImage> _swapchainImages;
    std::vector<VkImageView> _swapchainImageViews;
    VkExtent2D _swapchainExtent;

    FrameData _frames[FRAME_OVERLAP];
    FrameData& get_current_frame() { return _frames[_frameNumber % FRAME_OVERLAP]; }

    VkQueue _graphicsQueue;
    uint32_t _graphicsQueueFamily;

private:
    void init_vulkan();
    void init_swapchain();
    void init_commands();
    void init_sync_structures();

    void create_swapchain(uint32_t width, uint32_t height);
    void destroy_swapchain();
};


#endif //SHOWCASERENDERER_VK_ENGINE_H