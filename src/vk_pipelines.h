//
// Created by kyleb on 5/31/2026.
//

#ifndef SHOWCASERENDERER_VK_PIPELINES_H_
#define SHOWCASERENDERER_VK_PIPELINES_H_

#include <vulkan/vulkan_core.h>

class PipelineBuilder
{
public:
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo multisampling;
    VkPipelineLayout pipelineLayout;
    VkPipelineDepthStencilStateCreateInfo depthStencil;
    VkPipelineRenderingCreateInfo renderInfo;
    VkFormat colorAttachmentFormat;

    PipelineBuilder() { clear(); }

    void clear();

    VkPipeline build_pipeline(VkDevice device);
};

namespace vkutil {
bool load_shader_module(
    const char* filePath,
    VkDevice device,
    VkShaderModule* outShaderModule);
} // Namespace vkutil

#endif //SHOWCASERENDERER_VK_PIPELINES_H_