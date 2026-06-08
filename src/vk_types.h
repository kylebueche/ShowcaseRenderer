//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_TYPES_H
#define SHOWCASERENDERER_VK_TYPES_H

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <span>
#include <array>
#include <functional>
#include <deque>

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vk_mem_alloc.h>

#include <fmt/core.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#define VK_CHECK(x)                                                        \
    do {                                                                   \
        VkResult err = x;                                                  \
        if (err)                                                           \
        {                                                                  \
            fmt::print("Detected Vulkan error: {}", string_VkResult(err)); \
            std::abort();                                                  \
        }                                                                  \
    } while (0)

struct AllocatedImage
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

#endif //SHOWCASERENDERER_VK_TYPES_H