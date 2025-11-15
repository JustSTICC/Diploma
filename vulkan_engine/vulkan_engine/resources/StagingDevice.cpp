#include "../resources/StagingDevice.h"
#include "../config.h"
#include "../core/VulkanEngine.h"
#include "../rendering/CommandManager.h"
#include "../resources/BufferManager.h"
#include "../resources/TextureManager.h"
#include "../utils/ScopeExit.h"
#include "../utils/Utils.h"

StagingDevice::StagingDevice(VulkanEngine& eng)
    : eng_(eng){}


StagingDevice::MemoryRegionDesc StagingDevice::getNextFreeOffset(uint32_t size)
{
    const uint32_t requestedAlignedSize = getAlignedSize(
        size, STAGING_BUFFER_ALIGMENT_SIZE);
    ensureStagingBufferSize(requestedAlignedSize);
    auto bestNextIt = regions_.begin();
    for (auto it = regions_.begin(); it != regions_.end(); ++it) {
        if (eng_.commandManager_->isReady(it->handle_)) {
            if (it->size_ >= requestedAlignedSize) {
                const uint32_t unusedSize = it->size_ - requestedAlignedSize;
                const uint32_t unusedOffset = it->offset_ + requestedAlignedSize;
                SCOPE_EXIT{
                    regions_.erase(it);
                      if (unusedSize) {
                        regions_.push_front(
                          {unusedOffset, unusedSize, SubmitHandle()});
                      }
                };
                return { it->offset_, requestedAlignedSize, SubmitHandle() };
            }
            if (it->size_ > bestNextIt->size_) {
                bestNextIt = it;
            }
        }
    }
    if (bestNextIt != regions_.end() && eng_.commandManager_->isReady(bestNextIt->handle_)) {
        SCOPE_EXIT{
            regions_.erase(bestNextIt);
        };
        return { bestNextIt->offset_, bestNextIt->size_, SubmitHandle() };
    };
    waitAndReset();
    regions_.clear();
    const uint32_t unusedSize =
        stagingBufferSize_ > requestedAlignedSize ?
        stagingBufferSize_ - requestedAlignedSize : 0;
    if (unusedSize) {
        const uint32_t unusedOffset =
            stagingBufferSize_ - unusedSize;
        regions_.push_front({
            unusedOffset, unusedSize, SubmitHandle() });
    }
    return {
        .offset_ = 0,
        .size_ = stagingBufferSize_ - unusedSize,
        .handle_ = SubmitHandle() 
    };
}

void StagingDevice::waitAndReset() {
    for (const MemoryRegionDesc& r : regions_) {
        eng_.commandManager_->wait(r.handle_);
    };
    regions_.clear();
    regions_.push_front(
        { 0, stagingBufferSize_, SubmitHandle() });
}

void StagingDevice::bufferSubData(
    BufferManager& buffer,
    size_t dstOffset,
    size_t size,
    const void* data)
{
    if (buffer.isMapped()) {
        buffer.bufferSubData(eng_, dstOffset, size, data);
        return;
    }
    BufferManager* stagingBuffer = eng_.buffersPool_.get(stagingBuffer_);

    while (size) {
        MemoryRegionDesc desc = getNextFreeOffset((uint32_t)size);
        const uint32_t chunkSize =
            std::min((uint32_t)size, desc.size_);
        stagingBuffer->bufferSubData(eng_, desc.offset_, chunkSize, data);
        const CommandBufferWrapper& wrapper =
            eng_.commandManager_->acquire();
        const VkBufferCopy copy = {
          .srcOffset = desc.offset_,
          .dstOffset = dstOffset,
          .size = chunkSize };
        vkCmdCopyBuffer(wrapper.cmdBuf_, stagingBuffer->vkBuffer_,
            buffer.vkBuffer_, 1, &copy);
        VkBufferMemoryBarrier barrier = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
      .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
      .dstAccessMask = 0,
      .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
      .buffer = buffer.vkBuffer_,
      .offset = dstOffset,
      .size = chunkSize,
        };
        VkPipelineStageFlags dstMask =
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        if (buffer.getUsageFlags() &
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
            dstMask |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
            barrier.dstAccessMask |=
                VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }
        if (buffer.getUsageFlags() & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
            dstMask |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            barrier.dstAccessMask |= VK_ACCESS_INDEX_READ_BIT;
        }
        if (buffer.getUsageFlags() & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
            dstMask |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
            barrier.dstAccessMask |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
        vkCmdPipelineBarrier(wrapper.cmdBuf_,
            VK_PIPELINE_STAGE_TRANSFER_BIT, dstMask,
            VkDependencyFlags{}, 0, nullptr, 1, &barrier, 0, nullptr);
        desc.handle_ = eng_.commandManager_->submit(wrapper);
        regions_.push_back(desc);
        size -= chunkSize;
        data = (uint8_t*)data + chunkSize;
        dstOffset += chunkSize;
    }
}

void StagingDevice::imageData2D(
    TextureManager& image,
    const VkRect2D& imageRegion,
    uint32_t baseMipLevel,
    uint32_t numMipLevels,
    uint32_t layer,
    uint32_t numLayers,
    VkFormat format,
    const void* data)
{
    uint32_t width = image.getExtent().width >> baseMipLevel;
    uint32_t height = image.getExtent().height >> baseMipLevel;
    const Format_e texFormat(vkFormatToFormat(format));
    uint32_t layerStorageSize = 0;
    for (uint32_t i = 0; i < numMipLevels; ++i) {
        const uint32_t mipSize = TextureManager::getTextureBytesPerLayer(
            image.getExtent().width, image.getExtent().height,
            texFormat, i);
        layerStorageSize += mipSize;
        width = width <= 1 ? 1 : width >> 1;
        height = height <= 1 ? 1 : height >> 1;
    }
    const uint32_t storageSize = layerStorageSize * numLayers;
    ensureStagingBufferSize(storageSize);
    MemoryRegionDesc desc = getNextFreeOffset(storageSize);
    if (desc.size_ < storageSize) {
        waitAndReset();
        desc = getNextFreeOffset(storageSize);
    }
    VK_ASSERT(desc.size_ >= storageSize);
    const CommandBufferWrapper& wrapper = eng_.commandManager_->acquire();
    BufferManager* stagingBuffer = eng_.buffersPool_.get(stagingBuffer_);
    stagingBuffer->bufferSubData(eng_, desc.offset_, storageSize, data);
    uint32_t offset = 0;
    const uint32_t numPlanes = 1;
    VkImageAspectFlags imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
    for (uint32_t mipLevel = 0; mipLevel < numMipLevels; mipLevel++) {
        for (uint32_t layer = 0; layer != numLayers; layer++) {
            const uint32_t currentMipLevel = baseMipLevel + mipLevel;
            imageMemoryBarrier(
                wrapper.cmdBuf_,
                image.getVkImage(),
                StageAccess{ .stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, .access = VK_ACCESS_2_NONE },
                StageAccess{ .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VkImageSubresourceRange{ imageAspect, currentMipLevel, 1, layer, 1 });
            const VkExtent2D extent = getImagePlaneExtent({
        .width = std::max(1u, imageRegion.extent.width >> mipLevel),
        .height = std::max(1u, imageRegion.extent.height >> mipLevel),
                },
                vkFormatToFormat(format), 0);
            const VkRect2D region = {
              .offset = {.x = imageRegion.offset.x >> mipLevel,
                         .y = imageRegion.offset.y >> mipLevel},
              .extent = extent,
            };
            const VkBufferImageCopy copy = {
        .bufferOffset = desc.offset_ + offset,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource = VkImageSubresourceLayers{
          imageAspect, currentMipLevel, layer, 1},
        .imageOffset = {.x = region.offset.x,
                         .y = region.offset.y,
                         .z = 0},
        .imageExtent = {.width = region.extent.width,
                         .height = region.extent.height,
                         .depth = 1u },
            };
            vkCmdCopyBufferToImage(wrapper.cmdBuf_,
                stagingBuffer->vkBuffer_, image.getVkImage(),
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);
            imageMemoryBarrier(
                wrapper.cmdBuf_,
                image.getVkImage(),
                StageAccess{ .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
                StageAccess{ .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VkImageSubresourceRange{ imageAspect, currentMipLevel, 1, layer, 1 });
            offset += TextureManager::getTextureBytesPerLayer(
                imageRegion.extent.width, imageRegion.extent.height,
                texFormat, currentMipLevel);
        }
    }
    image.setCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    desc.handle_ = eng_.commandManager_->submit(wrapper);
    regions_.push_back(desc);
}

void StagingDevice::ensureStagingBufferSize(uint32_t sizeNeeded) {

    const uint32_t alignedSize = std::max(static_cast<uint32_t>(getAlignedSize(sizeNeeded, STAGING_BUFFER_ALIGMENT_SIZE)), minBufferSize_);

    sizeNeeded = alignedSize < eng_.config_.maxStagingBufferSize ? alignedSize : eng_.config_.maxStagingBufferSize;

    if (!stagingBuffer_.empty()) {
        const bool isEnoughSize = sizeNeeded <= stagingBufferSize_;
        const bool isMaxSize = stagingBufferSize_ == eng_.config_.maxStagingBufferSize;

        if (isEnoughSize || isMaxSize) {
            return;
        }
    }

    waitAndReset();

    // deallocate the previous staging buffer
    stagingBuffer_ = nullptr;

    // if the combined size of the new staging buffer and the existing one is larger than the limit imposed by some architectures on buffers
    // that are device and host visible, we need to wait for the current buffer to be destroyed before we can allocate a new one
    if ((sizeNeeded + stagingBufferSize_) > eng_.config_.maxStagingBufferSize) {
        eng_.waitDeferredTasks();
    }

    stagingBufferSize_ = sizeNeeded;

    char debugName[256] = { 0 };
    snprintf(debugName, sizeof(debugName) - 1, "Buffer: staging buffer %u", stagingBufferCounter_++);
    BufferDesc desc = {
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        .storage = StorageType_HostVisible,
        .size = stagingBufferSize_,
        .data = nullptr,
	};
	Result outResult;
    stagingBuffer_ = { &eng_, eng_.createBuffer(desc, debugName,&outResult) };
    VK_ASSERT(!stagingBuffer_.empty());

    regions_.clear();
    regions_.push_back({ 0, stagingBufferSize_, SubmitHandle() });
}
