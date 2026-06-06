//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_INITIALIZERS_H
#define SHOWCASERENDERER_VK_INITIALIZERS_H

#include "VkBootstrap.h"

namespace vkinit {
    VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);
    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count);
    VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags);
    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags);
    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags);
}


#endif //SHOWCASERENDERER_VK_INITIALIZERS_H