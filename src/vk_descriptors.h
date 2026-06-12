//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_DESCRIPTORS_H_
#define SHOWCASERENDERER_VK_DESCRIPTORS_H_

#include "vulkan/vulkan_core.h"
#include <vector>
#include <span>

struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void add_binding(uint32_t binding, VkDescriptorType type);
    void clear();
    VkDescriptorSetLayout build(
        VkDevice device,
        VkShaderStageFlags shaderStages,
        void* pNext,
        VkDescriptorSetLayoutCreateFlags flags);
};

struct DescriptorAllocator
{
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    VkDescriptorPool pool;
    void init_pool(VkDevice device, uint32_t maxSets, std::span<PoolSizeRatio> poolRatios);
    void clear_descriptors(VkDevice device);
    void destroy_pool(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout);
};


#endif //SHOWCASERENDERER_VK_DESCRIPTORS_H_