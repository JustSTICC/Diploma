#pragma once
#include "core/ICommandBuffer.h"
//#include "rendering/CommandManager.h"

class VulkanEngine;

class CommandBuffer : public ICommandBuffer
{
public:
    explicit CommandBuffer(VulkanEngine* eng);
    ~CommandBuffer() override;

    CommandBuffer& operator=(CommandBuffer&& other) = default;

    operator VkCommandBuffer() const {
        return getVkCommandBuffer();
    }

    void transitionToShaderReadOnly(TextureHandle surface) const override;

    //void cmdBindRayTracingPipeline(RayTracingPipelineHandle handle) override;

    //void cmdBindComputePipeline(ComputePipelineHandle handle) override;
    void cmdDispatchThreadGroups(const Dimensions& threadgroupCount, const Dependencies& deps) override;

    void cmdPushDebugGroupLabel(const char* label, uint32_t colorRGBA) const override;
    void cmdInsertDebugEventLabel(const char* label, uint32_t colorRGBA) const override;
    void cmdPopDebugGroupLabel() const override;

    void cmdBeginRendering(const RenderDesc& renderPass, const Framebuffer& desc, const Dependencies& deps) override;
    void cmdEndRendering() override;

    void cmdBindViewport(const Viewport& viewport) override;
    void cmdBindScissorRect(const ScissorRect& rect) override;

    void cmdBindRenderPipeline(RenderPipelineHandle handle) override;
    void cmdBindDepthState(const DepthState& state) override;

    void cmdBindVertexBuffer(uint32_t index, BufferHandle buffer, uint64_t bufferOffset) override;
    void cmdBindIndexBuffer(BufferHandle indexBuffer, IndexFormat_e indexFormat, uint64_t indexBufferOffset) override;
    void cmdPushConstants(const void* data, size_t size, size_t offset) override;

    void cmdFillBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, uint32_t data) override;
    void cmdUpdateBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, const void* data) override;

    void cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance) override;
    void cmdDrawIndexed(uint32_t indexCount,
        uint32_t instanceCount,
        uint32_t firstIndex,
        int32_t vertexOffset,
        uint32_t baseInstance) override;
    void cmdDrawIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) override;
    void cmdDrawIndexedIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) override;
    void cmdDrawIndexedIndirectCount(BufferHandle indirectBuffer,
        size_t indirectBufferOffset,
        BufferHandle countBuffer,
        size_t countBufferOffset,
        uint32_t maxDrawCount,
        uint32_t stride = 0) override;
    /*void cmdDrawMeshTasks(const Dimensions& threadgroupCount) override;
    void cmdDrawMeshTasksIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) override;
    void cmdDrawMeshTasksIndirectCount(BufferHandle indirectBuffer,
        size_t indirectBufferOffset,
        BufferHandle countBuffer,
        size_t countBufferOffset,
        uint32_t maxDrawCount,
        uint32_t stride = 0) override;*/
    //void cmdTraceRays(uint32_t width, uint32_t height, uint32_t depth, const Dependencies& deps) override;

    void cmdSetBlendColor(const float color[4]) override;
    void cmdSetDepthBias(float constantFactor, float slopeFactor, float clamp) override;
    void cmdSetDepthBiasEnable(bool enable) override;

    void cmdResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount) override;
    void cmdWriteTimestamp(QueryPoolHandle pool, uint32_t query) override;

    void cmdClearColorImage(TextureHandle tex, const VkClearColorValue& value, const TextureLayers& layers) override;
    void cmdCopyImage(TextureHandle src,
        TextureHandle dst,
        const Dimensions& extent,
        const VkOffset3D& srcOffset,
        const VkOffset3D& dstOffset,
        const TextureLayers& srcLayers,
        const TextureLayers& dstLayers) override;
    void cmdGenerateMipmap(TextureHandle handle) override;
    //void cmdUpdateTLAS(AccelStructHandle handle, BufferHandle instancesBuffer) override;

    VkCommandBuffer getVkCommandBuffer() const {
        return wrapper_ ? wrapper_->cmdBuf_ : VK_NULL_HANDLE;
    }

private:
    //void useComputeTexture(TextureHandle texture, VkPipelineStageFlags2 dstStage);
    void bufferBarrier(BufferHandle handle, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage);

private:
    friend class VulkanEngine;

    VulkanEngine* eng_ = nullptr;
    const CommandBufferWrapper* wrapper_ = nullptr;

    Framebuffer framebuffer_ = {};
    SubmitHandle lastSubmitHandle_ = {};

    VkPipeline lastPipelineBound_ = VK_NULL_HANDLE;

    bool isRendering_ = false;
    uint32_t viewMask_ = 0;

    RenderPipelineHandle currentPipelineGraphics_ = {};
    /*ComputePipelineHandle currentPipelineCompute_ = {};
    RayTracingPipelineHandle currentPipelineRayTracing_ = {};*/

};

