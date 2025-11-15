#pragma once
#include "../core/IVkEngine.h"
#include <deque>

#define STAGING_BUFFER_ALIGMENT_SIZE 16 

class VulkanEngine;
class BufferManager;
class TextureManager;

class StagingDevice final
{
public:
    explicit StagingDevice(VulkanEngine& eng);
    void bufferSubData(BufferManager& buffer,
        size_t dstOffset, size_t size, const void* data);
    void imageData2D(TextureManager& texture,
        const VkRect2D& imageRegion,
        uint32_t baseMipLevel,
        uint32_t numMipLevels,
        uint32_t layer,
        uint32_t numLayers,
        VkFormat format,
        const void* data);
private:
    struct MemoryRegionDesc {
        uint32_t offset_ = 0;
        uint32_t size_ = 0;
        SubmitHandle handle_ = {};
    };

    MemoryRegionDesc getNextFreeOffset(uint32_t size);
    void ensureStagingBufferSize(uint32_t sizeNeeded);
    void waitAndReset();
private:
    VulkanEngine& eng_;
    Holder<BufferHandle> stagingBuffer_;
    uint32_t stagingBufferSize_ = 0;
    uint32_t stagingBufferCounter_ = 0;
    uint32_t maxBufferSize_ = 0;
    const uint32_t minBufferSize_ = 4u * 2048u * 2048u;
    std::deque<MemoryRegionDesc> regions_;
};

