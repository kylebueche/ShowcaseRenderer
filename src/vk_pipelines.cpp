//
// Created by kyleb on 6/11/2026.
//

#include "vk_pipelines.h"
#include <fstream>
#include <vk_initializers.h>

namespace vkutil {
bool load_shader_module(
    const char* filePath,
    VkDevice device,
    VkShaderModule* outShaderModule) {

    std::ifstream file(filePath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        return false;
    }

    // Find the size of the file in bytes by looking at the cursor (at the end).
    size_t fileSize = file.tellg();

    // Spir-v expects the buffer to be uint32 format
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // Put cursor at beginning
    file.seekg(0);

    // load entire file into the buffer
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();

    // Create a new shader module
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}

} // Namespace vkutil
