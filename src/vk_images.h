//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_IMAGES_H_
#define SHOWCASERENDERER_VK_IMAGES_H_

#include <vulkan/vulkan.h>

namespace vkutil {

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
void copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);

}

#endif //SHOWCASERENDERER_VK_IMAGES_H_