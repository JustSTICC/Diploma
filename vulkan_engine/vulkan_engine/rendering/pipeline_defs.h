#pragma once
#include <vulkan/vulkan.hpp>
#include <cstdint>

#define VK_MAX_COLOR_ATTACHMENTS 8
#define VK_SPECIALIZATION_CONSTANTS_MAX 16


struct SpecializationConstantEntry {
    uint32_t constantId = 0;
    uint32_t offset = 0; // offset within SpecializationConstantDesc::data
    size_t size = 0;
};

struct SpecializationConstantDesc {
    SpecializationConstantEntry entries[VK_SPECIALIZATION_CONSTANTS_MAX] = {};
    const void* data = nullptr;
    size_t dataSize = 0;
    uint32_t getNumSpecializationConstants() const {
        uint32_t n = 0;
        while (n < VK_SPECIALIZATION_CONSTANTS_MAX && this->entries[n].size) {
            n++;
        }
        return n;
    }
};

struct ColorAttachment {
    VkFormat format = VK_FORMAT_UNDEFINED;
    bool blendEnabled = false;
    VkBlendOp rgbBlendOp = VK_BLEND_OP_ADD;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcRGBBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstRGBBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
};

struct StencilState {
    VkStencilOp stencilFailureOp = VK_STENCIL_OP_KEEP;
    VkStencilOp depthFailureOp = VK_STENCIL_OP_KEEP;
    VkStencilOp depthStencilPassOp = VK_STENCIL_OP_KEEP;
    VkCompareOp stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    uint32_t readMask = (uint32_t)~0;
    uint32_t writeMask = (uint32_t)~0;
};
