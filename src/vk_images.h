//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_IMAGES_H_
#define SHOWCASERENDERER_VK_IMAGES_H_

#include <vulkan/vulkan.h>

namespace vkutil {

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);

}

#endif //SHOWCASERENDERER_VK_IMAGES_H_