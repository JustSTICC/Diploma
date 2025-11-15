#pragma once
#include "../common/render_def.h"

class ICommandBuffer {
public:
    virtual ~ICommandBuffer() = default;

    virtual void transitionToShaderReadOnly(TextureHandle surface) const = 0;

    virtual void cmdPushDebugGroupLabel(const char* label, uint32_t colorRGBA = 0xffffffff) const = 0;
    virtual void cmdInsertDebugEventLabel(const char* label, uint32_t colorRGBA = 0xffffffff) const = 0;
    virtual void cmdPopDebugGroupLabel() const = 0;

    //virtual void cmdBindRayTracingPipeline(RayTracingPipelineHandle handle) = 0;

    //virtual void cmdBindComputePipeline(ComputePipelineHandle handle) = 0;
    virtual void cmdDispatchThreadGroups(const Dimensions& threadgroupCount, const Dependencies& deps = {}) = 0;

    virtual void cmdBeginRendering(const RenderDesc& renderPass, const Framebuffer& desc, const Dependencies& deps = {}) = 0;
    virtual void cmdEndRendering() = 0;

    virtual void cmdBindViewport(const Viewport& viewport) = 0;
    virtual void cmdBindScissorRect(const ScissorRect& rect) = 0;

    virtual void cmdBindRenderPipeline(RenderPipelineHandle handle) = 0;
    virtual void cmdBindDepthState(const DepthState& state) = 0;

    virtual void cmdBindVertexBuffer(uint32_t index, BufferHandle buffer, uint64_t bufferOffset = 0) = 0;
    virtual void cmdBindIndexBuffer(BufferHandle indexBuffer, IndexFormat_e indexFormat, uint64_t indexBufferOffset = 0) = 0;
    virtual void cmdPushConstants(const void* data, size_t size, size_t offset = 0) = 0;
    template<typename Struct>
    void cmdPushConstants(const Struct& data, size_t offset = 0) {
        this->cmdPushConstants(&data, sizeof(Struct), offset);
    }

    virtual void cmdFillBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, uint32_t data) = 0;
    virtual void cmdUpdateBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, const void* data) = 0;
    template<typename Struct>
    void cmdUpdateBuffer(BufferHandle buffer, const Struct& data, size_t bufferOffset = 0) {
        this->cmdUpdateBuffer(buffer, bufferOffset, sizeof(Struct), &data);
    }

    virtual void cmdDraw(uint32_t vertexCount, uint32_t instanceCount = 1, uint32_t firstVertex = 0, uint32_t baseInstance = 0) = 0;
    virtual void cmdDrawIndexed(uint32_t indexCount,
        uint32_t instanceCount = 1,
        uint32_t firstIndex = 0,
        int32_t vertexOffset = 0,
        uint32_t baseInstance = 0) = 0;
    virtual void cmdDrawIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride = 0) = 0;
    virtual void cmdDrawIndexedIndirect(BufferHandle indirectBuffer,
        size_t indirectBufferOffset,
        uint32_t drawCount,
        uint32_t stride = 0) = 0;
    virtual void cmdDrawIndexedIndirectCount(BufferHandle indirectBuffer,
        size_t indirectBufferOffset,
        BufferHandle countBuffer,
        size_t countBufferOffset,
        uint32_t maxDrawCount,
        uint32_t stride = 0) = 0;
    /*virtual void cmdDrawMeshTasks(const Dimensions& threadgroupCount) = 0;
    virtual void cmdDrawMeshTasksIndirect(BufferHandle indirectBuffer,
        size_t indirectBufferOffset,
        uint32_t drawCount,
        uint32_t stride = 0) = 0;*/
    /*virtual void cmdDrawMeshTasksIndirectCount(BufferHandle indirectBuffer,
        size_t indirectBufferOffset,
        BufferHandle countBuffer,
        size_t countBufferOffset,
        uint32_t maxDrawCount,
        uint32_t stride = 0) = 0;*/
    //virtual void cmdTraceRays(uint32_t width, uint32_t height, uint32_t depth = 1, const Dependencies& deps = {}) = 0;

    virtual void cmdSetBlendColor(const float color[4]) = 0;
    // the argument order is correct, so the `clamp` parameter can have a default value
    virtual void cmdSetDepthBias(float constantFactor, float slopeFactor, float clamp = 0.0f) = 0;
    virtual void cmdSetDepthBiasEnable(bool enable) = 0;

    virtual void cmdResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount) = 0;
    virtual void cmdWriteTimestamp(QueryPoolHandle pool, uint32_t query) = 0;

    virtual void cmdClearColorImage(TextureHandle tex, const VkClearColorValue& value, const TextureLayers& layers = {}) = 0;
    virtual void cmdCopyImage(TextureHandle src,
        TextureHandle dst,
        const Dimensions& extent,
        const VkOffset3D& srcOffset = {},
        const VkOffset3D& dstOffset = {},
        const TextureLayers& srcLayers = {},
        const TextureLayers& dstLayers = {}) = 0;
    virtual void cmdGenerateMipmap(TextureHandle handle) = 0;
    //virtual void cmdUpdateTLAS(AccelStructHandle handle, BufferHandle instancesBuffer) = 0;
};
