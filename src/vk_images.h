// Copyright (c) 2026 Kyle Bueche.
// Author: Kyle Bueche.
// This project is licensed under the MIT Licence - see LICENSE.txt.
// No warranty implied.

#ifndef SHOWCASERENDERER_VK_IMAGES_H_
#define SHOWCASERENDERER_VK_IMAGES_H_

#include <vulkan/vulkan.h>

namespace vkutil {

void transition_image(VkCommandBuffer cmd, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout);
void copy_image_to_image(VkCommandBuffer cmd, VkImage src, VkImage dst, VkExtent2D srcSize, VkExtent2D dstSize);

}

#endif //SHOWCASERENDERER_VK_IMAGES_H_