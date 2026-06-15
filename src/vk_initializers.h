//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_INITIALIZERS_H_
#define SHOWCASERENDERER_VK_INITIALIZERS_H_

#include "VkBootstrap.h"

namespace vkinit {

VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags);
VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count);
VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags);
VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags);
VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags);
VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);
VkSemaphoreSubmitInfo semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);
VkSubmitInfo2 submit_info(VkCommandBufferSubmitInfo* cmdInfo, VkSemaphoreSubmitInfo* signalSemaphoreInfo, VkSemaphoreSubmitInfo* waitSemaphoreInfo);
VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
VkImageViewCreateInfo image_view_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);
VkRenderingAttachmentInfo attachment_info(VkImageView view, VkClearValue* clear, VkImageLayout layout);
VkRenderingInfo rendering_info(VkExtent2D extent, VkRenderingAttachmentInfo* attachmentInfo);
}


#endif //SHOWCASERENDERER_VK_INITIALIZERS_H_