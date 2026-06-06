//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_IMAGES_H
#define SHOWCASERENDERER_VK_IMAGES_H

#include <vulkan/vulkan.h>

namespace vkutil {
    void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);
}

#endif //SHOWCASERENDERER_VK_IMAGES_H