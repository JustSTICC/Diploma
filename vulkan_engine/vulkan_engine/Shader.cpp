#include "Shader.h"
#include <fstream>
#include <iostream>
#include "FilePaths.h"


std::vector<VkShaderEXT> Shader::createShaderModule(VkDevice device) {

    std::vector<VkShaderCreateInfoEXT> shaderInfo(2);
    shaderInfo[0] = createVertexShaderInfo();
    shaderInfo[1] = createFragmentShaderInfo();

    std::vector<VkShaderEXT> shaders(shaderInfo.size());
    //auto result = vkCreateShadersEXT(device, shaderInfo.size(), shaderInfo.data(), nullptr, shaders.data());
 
   /* if (result == VK_SUCCESS) {
        std::cout << "Shader modules created successfully - " << result << std::endl;

        return shaders;
    }
    else {
        std::cerr << "Failed to create shader modules - " << result << std::endl;
        throw std::runtime_error("failed to create shader modules!");
    }*/
    return shaders;
}


VkShaderCreateInfoEXT Shader::createVertexShaderInfo() {
    VkShaderCreateFlagsEXT flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
    VkShaderStageFlags nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<char> vertexSrc = readFile(VERTEX_SHADER_PATH);
    VkShaderCodeTypeEXT codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
    const char* pName = "main";
    VkShaderCreateInfoEXT vertexInfo{};
    vertexInfo.flags = flags;
    vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexInfo.nextStage = nextStage;
    vertexInfo.codeType = codeType;
    vertexInfo.codeSize = vertexSrc.size();
    vertexInfo.pCode = vertexSrc.data();
    vertexInfo.pName = pName;
    return vertexInfo;
}

VkShaderCreateInfoEXT Shader::createFragmentShaderInfo() {

    VkShaderCreateFlagsEXT flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
    VkShaderStageFlags nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<char> fragmentSrc = readFile(FRAGMENT_SHADER_PATH);

    VkShaderCodeTypeEXT codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
    const char* pName = "main";
    VkShaderCreateInfoEXT fragmentInfo{};
    fragmentInfo.flags = flags;
    fragmentInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentInfo.codeType = codeType;
    fragmentInfo.codeSize = fragmentSrc.size();
    fragmentInfo.pCode = fragmentSrc.data();
    fragmentInfo.pName = pName;
    return fragmentInfo;
}

std::vector<char> Shader::readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

