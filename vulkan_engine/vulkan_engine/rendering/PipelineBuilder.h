#pragma once
#include "../common/render_def.h"
class PipelineBuilder
{
public:
    PipelineBuilder();
    ~PipelineBuilder() = default;

    PipelineBuilder& dynamicState(VkDynamicState state);
    PipelineBuilder& primitiveTopology(VkPrimitiveTopology topology);
    PipelineBuilder& rasterizationSamples(VkSampleCountFlagBits samples, float minSampleShading);
    PipelineBuilder& shaderStage(VkPipelineShaderStageCreateInfo stage);
    PipelineBuilder& stencilStateOps(VkStencilFaceFlags faceMask,
        VkStencilOp failOp,
        VkStencilOp passOp,
        VkStencilOp depthFailOp,
        VkCompareOp compareOp);
    PipelineBuilder& stencilMasks(VkStencilFaceFlags faceMask, uint32_t compareMask, uint32_t writeMask, uint32_t reference);
    PipelineBuilder& cullMode(VkCullModeFlags mode);
    PipelineBuilder& frontFace(VkFrontFace mode);
    PipelineBuilder& polygonMode(VkPolygonMode mode);
    PipelineBuilder& vertexInputState(const VkPipelineVertexInputStateCreateInfo& state);
    PipelineBuilder& viewMask(uint32_t mask);
    PipelineBuilder& colorAttachments(const VkPipelineColorBlendAttachmentState* states,
        const VkFormat* formats,
        uint32_t numColorAttachments);
    PipelineBuilder& depthAttachmentFormat(VkFormat format);
    PipelineBuilder& stencilAttachmentFormat(VkFormat format);
    PipelineBuilder& patchControlPoints(uint32_t numPoints);

    VkResult build(VkDevice device,
        VkPipelineCache pipelineCache,
        VkPipelineLayout pipelineLayout,
        VkPipeline* outPipeline,
        const char* debugName = nullptr) noexcept;

    static uint32_t getNumPipelinesCreated() {
        return numPipelinesCreated_;
    }

private:
    enum { VK_MAX_DYNAMIC_STATES = 128 };
    uint32_t numDynamicStates_ = 0;
    VkDynamicState dynamicStates_[VK_MAX_DYNAMIC_STATES] = {};

    uint32_t numShaderStages_ = 0;
    VkPipelineShaderStageCreateInfo shaderStages_[5] = {};

    VkPipelineVertexInputStateCreateInfo vertexInputState_;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly_;
    VkPipelineRasterizationStateCreateInfo rasterizationState_;
    VkPipelineMultisampleStateCreateInfo multisampleState_;
    VkPipelineDepthStencilStateCreateInfo depthStencilState_;
    VkPipelineTessellationStateCreateInfo tessellationState_;

    uint32_t viewMask_ = 0;
    uint32_t numColorAttachments_ = 0;
    VkPipelineColorBlendAttachmentState colorBlendAttachmentStates_[VK_MAX_COLOR_ATTACHMENTS] = {};
    VkFormat colorAttachmentFormats_[VK_MAX_COLOR_ATTACHMENTS] = {};

    VkFormat depthAttachmentFormat_ = VK_FORMAT_UNDEFINED;
    VkFormat stencilAttachmentFormat_ = VK_FORMAT_UNDEFINED;

    inline static uint32_t numPipelinesCreated_ = 0;
};

