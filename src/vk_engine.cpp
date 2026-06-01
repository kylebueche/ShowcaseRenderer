//
// Created by kyleb on 6/1/2026.
//

#include "vk_engine.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <vk_initializers.h>
#include <vk_types.h>

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

    _isInitialized = true;
}

void VulkanEngine::cleanup() {
    if (_isInitialized) {
        SDL_DestroyWindow(_window);
    }

    loadedEngine = nullptr;
}

void VulkanEngine::draw() {
    // Nothing yet
}

void VulkanEngine::run() {
    SDL_Event e;
    bool bQuit = false;

    // Main Loop
    while (!bQuit) {
        while (SDL_PollEvent(&e) != 0) {
            // Close the window on alt-f4 or x button
            if (e.type == SDL_QUIT)
                bQuit = true;

            if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOW_EVENT_MINIMIZED) {
                    stop_rendering = true;
                } else if (e.window.event == SDL_WINDOW_EVENT_RESTORED) {
                    stop_rendering = false;
                }
            }
        }

        if (stop_rendering) {
            // Throttle speed
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
    }
}