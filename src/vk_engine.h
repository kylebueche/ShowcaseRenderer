//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_ENGINE_H
#define SHOWCASERENDERER_VK_ENGINE_H

#include "vk_types.h"

class VulkanEngine
{
public:

    bool _isInitialized{ false };
    int _frameNumber {0};
    bool stop_rendering{ false };
    VKExtent2d _windowExtent{ 1600, 900 };

    struct SDL_Window* _window{ nullptr };
    static VulkanEngine& Get();

    void init();
    void cleanup();
    void draw(); // Draw loop
    void run(); // Main loop
};


#endif //SHOWCASERENDERER_VK_ENGINE_H