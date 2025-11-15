#include "VulkanGraphicsPipelineV2.h"
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <array>
#include <glm/glm.hpp>
#include "../common/Vertex.h"
#include <future>
#include "../utils/Utils.h"

VulkanGraphicsPipelineV2::VulkanGraphicsPipelineV2() {}

VulkanGraphicsPipelineV2::~VulkanGraphicsPipelineV2() {
    cleanup();
}

void VulkanGraphicsPipelineV2::cleanup() {
    // Free the specification constant data storage if allocated
    if (specConstantDataStorage_) {
        free(specConstantDataStorage_);
        specConstantDataStorage_ = nullptr;
    }
    
    // Note: Vulkan objects (pipeline, pipelineLayout, pipelineCache) should be 
    // destroyed by the VulkanEngine, not by this class, since they are managed 
    // by the Pool and VulkanEngine lifetime
    pipeline_ = VK_NULL_HANDLE;
    pipelineLayout_ = VK_NULL_HANDLE;
    pipelineCache_ = VK_NULL_HANDLE;
}

void VulkanGraphicsPipelineV2::createRenderPipeline(const PipelineDesc& pipeDesc) {

    const bool hasColorAttachments = pipeDesc.getNumColorAttachments() > 0;
    const bool hasDepthAttachment = pipeDesc.depthFormat != VK_FORMAT_UNDEFINED;
    const bool hasAnyAttachments = hasColorAttachments || hasDepthAttachment;
    if (!VK_VERIFY(hasAnyAttachments)) return ;
    if (!VK_VERIFY(pipeDesc.smVert.valid())) return;
    if (!VK_VERIFY(pipeDesc.smFrag.valid())) return ;
    desc_ = pipeDesc;
    const VertexInput& vstate = desc_.vertexInput;
    bool bufferAlreadyBound[VK_VERTEX_BUFFER_MAX] = {};
    numAttributes_ = vstate.getNumAttributes();
    for (uint32_t i = 0; i != numAttributes_; i++) {
        const VertexInput::VertexAttribute& attr = vstate.attributes[i];
        vkAttributes_[i] = { .location = attr.location,
                                 .binding = attr.binding,
                                 .format = attr.format,
                                 .offset = (uint32_t)attr.offset };
        if (!bufferAlreadyBound[attr.binding]) {
            bufferAlreadyBound[attr.binding] = true;
            vkBindings_[numBindings_++] = {
              .binding = attr.binding,
              .stride = vstate.inputBindings[attr.binding].stride,
              .inputRate = VK_VERTEX_INPUT_RATE_VERTEX 
            };
        }   
    }
    if (pipeDesc.specInfo.data && pipeDesc.specInfo.dataSize) {
        specConstantDataStorage_ =
            malloc(pipeDesc.specInfo.dataSize);
        memcpy(specConstantDataStorage_,
            pipeDesc.specInfo.data, pipeDesc.specInfo.dataSize);
        desc_.specInfo.data = specConstantDataStorage_;
    }
}

void VulkanGraphicsPipelineV2::getVkPipeline(Pool<ShaderModule, Shader>& shaderModulesPool,
    VkDescriptorSetLayout vkDSL,
    VkDevice device,
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo,
    VkPhysicalDeviceLimits limits) const{
    // Implementation of setting up the Vulkan pipeline goes here.
    
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    const PipelineDesc& desc = desc_;
    const uint32_t numColorAttachments =
        desc_.getNumColorAttachments();
    VkPipelineColorBlendAttachmentState
        colorBlendAttachmentStates[VK_MAX_COLOR_ATTACHMENTS] = {};
    VkFormat colorAttachmentFormats[VK_MAX_COLOR_ATTACHMENTS] = {};
    for (uint32_t i = 0; i != numColorAttachments; i++) {
        const ColorAttachment& attachment = desc_.color[i];
        colorAttachmentFormats[i] = attachment.format;
        if (!attachment.blendEnabled) {
            colorBlendAttachmentStates[i] =
                VkPipelineColorBlendAttachmentState{
                  .blendEnable = VK_FALSE,
                  .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                  .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
                  .colorBlendOp = VK_BLEND_OP_ADD,
                  .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                  .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                  .alphaBlendOp = VK_BLEND_OP_ADD,
                  .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                    VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT,
            };
        }
        else {
            colorBlendAttachmentStates[i] =
                VkPipelineColorBlendAttachmentState{
                  .blendEnable = VK_TRUE,
                  .srcColorBlendFactor = attachment.srcRGBBlendFactor,
                  .dstColorBlendFactor = attachment.dstRGBBlendFactor,
                  .colorBlendOp = attachment.rgbBlendOp,
                  .srcAlphaBlendFactor = attachment.srcAlphaBlendFactor,
                  .dstAlphaBlendFactor = attachment.dstAlphaBlendFactor,
                  .alphaBlendOp = attachment.alphaBlendOp,
                  .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                    VK_COLOR_COMPONENT_G_BIT |
                                    VK_COLOR_COMPONENT_B_BIT |
                                    VK_COLOR_COMPONENT_A_BIT,
            };
        }
    }
    const Shader* vert = shaderModulesPool.get(desc_.smVert);
    const Shader* geom = shaderModulesPool.get(desc_.smGeom);
    const Shader* frag = shaderModulesPool.get(desc_.smFrag);
    const VkPipelineVertexInputStateCreateInfo ciVertexInputState =
    {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .vertexBindingDescriptionCount = numBindings_,
      .pVertexBindingDescriptions = numBindings_ ? vkBindings_ : nullptr,
      .vertexAttributeDescriptionCount = numAttributes_,
      .pVertexAttributeDescriptions = numAttributes_ ? vkAttributes_ : nullptr,
    };
#define UPDATE_PUSH_CONSTANT_SIZE(sm, bit) if (sm) { \
  pushConstantsSize = std::max(pushConstantsSize,    \
  sm->pushConstantsSize);                            \
  shaderStageFlags_ |= bit; }
    shaderStageFlags_ = 0;
    uint32_t pushConstantsSize = 0;
    UPDATE_PUSH_CONSTANT_SIZE(vert,
        VK_SHADER_STAGE_VERTEX_BIT);
    //UPDATE_PUSH_CONSTANT_SIZE(tesc,
    //    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);
    //UPDATE_PUSH_CONSTANT_SIZE(tese,
    //    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    UPDATE_PUSH_CONSTANT_SIZE(geom,
        VK_SHADER_STAGE_GEOMETRY_BIT);
    UPDATE_PUSH_CONSTANT_SIZE(frag,
        VK_SHADER_STAGE_FRAGMENT_BIT);
#undef UPDATE_PUSH_CONSTANT_SIZE

    VkSpecializationMapEntry entries[VK_SPECIALIZATION_CONSTANTS_MAX] = {};
    const VkSpecializationInfo si = getPipelineShaderStageSpecializationInfo(desc.specInfo, entries);
    const VkDescriptorSetLayout dsls[4] =
    { vkDSL, vkDSL, vkDSL, vkDSL };
    const VkPushConstantRange range = {
      .stageFlags = shaderStageFlags_,
      .offset = 0,
      .size = pushConstantsSize,
    };
    const VkPipelineLayoutCreateInfo ci = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = (uint32_t)VK_UTILS_GET_ARRAY_SIZE(dsls),
      .pSetLayouts = dsls,
      .pushConstantRangeCount = pushConstantsSize ? 1u : 0u,
      .pPushConstantRanges = pushConstantsSize ? &range : nullptr,
    };
    ASSERT_VK_RESULT(vkCreatePipelineLayout(device, &ci, nullptr, &layout), "Creating Pipeline Layout");

    PipelineBuilder()
        // from Vulkan 1.0
        .dynamicState(VK_DYNAMIC_STATE_VIEWPORT)
        .dynamicState(VK_DYNAMIC_STATE_SCISSOR)
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS)
        .dynamicState(VK_DYNAMIC_STATE_BLEND_CONSTANTS)
        // from Vulkan 1.3
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE)
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE)
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_COMPARE_OP)
        .dynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE)
        .primitiveTopology(desc.topology)
        .rasterizationSamples(getVulkanSampleCountFlags(desc.samplesCount,
            (limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts)),
            desc.minSampleShading)
        .polygonMode(desc_.polygonMode)
        .stencilStateOps(VK_STENCIL_FACE_FRONT_BIT,
            desc_.frontFaceStencil.stencilFailureOp,
            desc_.frontFaceStencil.depthStencilPassOp,
            desc_.frontFaceStencil.depthFailureOp,
            desc_.frontFaceStencil.stencilCompareOp)
        .stencilStateOps(VK_STENCIL_FACE_BACK_BIT,
            desc_.backFaceStencil.stencilFailureOp,
            desc_.backFaceStencil.depthStencilPassOp,
            desc_.backFaceStencil.depthFailureOp,
            desc_.backFaceStencil.stencilCompareOp)
        .stencilMasks(VK_STENCIL_FACE_FRONT_BIT, 0xFF,
            desc_.frontFaceStencil.writeMask,
            desc_.frontFaceStencil.readMask)
        .stencilMasks(VK_STENCIL_FACE_BACK_BIT, 0xFF,
            desc_.backFaceStencil.writeMask,
            desc_.backFaceStencil.readMask)
        .shaderStage(getPipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_VERTEX_BIT,
            vert->shaderModule_, desc.entryPointVert, &si))
        .shaderStage(getPipelineShaderStageCreateInfo(
            VK_SHADER_STAGE_FRAGMENT_BIT,
            frag->shaderModule_, desc.entryPointFrag, &si))
        .shaderStage(geom ?
            getPipelineShaderStageCreateInfo(
                VK_SHADER_STAGE_GEOMETRY_BIT,
                geom->shaderModule_, desc.entryPointGeom, &si) : VkPipelineShaderStageCreateInfo{ .module = VK_NULL_HANDLE })
        .cullMode(desc_.cullMode)
        .frontFace(desc_.frontFaceWinding)
        .vertexInputState(vertexInputStateCreateInfo)
        .colorAttachments(colorBlendAttachmentStates,colorAttachmentFormats, numColorAttachments)
        .depthAttachmentFormat(desc.depthFormat)
        .stencilAttachmentFormat(desc.stencilFormat)
        .patchControlPoints(desc.patchControlPoints)
        .build(device, pipelineCache_, layout, &pipeline);
    pipeline_ = pipeline;
    pipelineLayout_ = layout;
}

VkSpecializationInfo VulkanGraphicsPipelineV2::getPipelineShaderStageSpecializationInfo(SpecializationConstantDesc desc, VkSpecializationMapEntry* outEntries) const{
    const uint32_t numEntries = desc.getNumSpecializationConstants();
    if (outEntries) {
        for (uint32_t i = 0; i != numEntries; i++) {
            outEntries[i] = VkSpecializationMapEntry{
                .constantID = desc.entries[i].constantId,
                .offset = desc.entries[i].offset,
                .size = desc.entries[i].size,
            };
        }
    }
    return VkSpecializationInfo{
        .mapEntryCount = numEntries,
        .pMapEntries = outEntries,
        .dataSize = desc.dataSize,
        .pData = desc.data,
    };
}


VkPipelineShaderStageCreateInfo VulkanGraphicsPipelineV2::getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
    VkShaderModule shaderModule,
    const char* entryPoint,
    const VkSpecializationInfo* specializationInfo) const {
    return VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .flags = 0,
        .stage = stage,
        .module = shaderModule,
        .pName = entryPoint ? entryPoint : "main",
        .pSpecializationInfo = specializationInfo,
    };
}

