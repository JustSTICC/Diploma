#include "TextureManager.h"
#include "../core/VulkanEngine.h"
#include "../core/VulkanDevice.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stb_image_resize2.h>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <ktx.h>
#include <KTX-Software/lib/src/vkformat_enum.h>
#include <KTX-Software/lib/src/gl_format.h>
#include "../utils/Utils.h"




TextureManager::TextureManager(VulkanDevice* vulkanDevice) : vulkanDevice_(vulkanDevice) {}

TextureManager::~TextureManager(){
    cleanup();
}

static constexpr TextureFormatProperties properties[] = {
    PROPS(Invalid, 1),
    PROPS(R_UN8, 1),
    PROPS(R_UI16, 2),
    PROPS(R_UI32, 4),
    PROPS(R_UN16, 2),
    PROPS(R_F16, 2),
    PROPS(R_F32, 4),
    PROPS(RG_UN8, 2),
    PROPS(RG_UI16, 4),
    PROPS(RG_UI32, 8),
    PROPS(RG_UN16, 4),
    PROPS(RG_F16, 4),
    PROPS(RG_F32, 8),
    PROPS(RGBA_UN8, 4),
    PROPS(RGBA_UI32, 16),
    PROPS(RGBA_F16, 8),
    PROPS(RGBA_F32, 16),
    PROPS(RGBA_SRGB8, 4),
    PROPS(BGRA_UN8, 4),
    PROPS(BGRA_SRGB8, 4),
    PROPS(A2B10G10R10_UN, 4),
    PROPS(A2R10G10B10_UN, 4),
    PROPS(ETC2_RGB8, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(ETC2_SRGB8, 8, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(BC7_RGBA, 16, .blockWidth = 4, .blockHeight = 4, .compressed = true),
    PROPS(Z_UN16, 2, .depth = true),
    PROPS(Z_UN24, 3, .depth = true),
    PROPS(Z_F32, 4, .depth = true),
    PROPS(Z_UN24_S_UI8, 4, .depth = true, .stencil = true),
    PROPS(Z_F32_S_UI8, 5, .depth = true, .stencil = true),
    PROPS(YUV_NV12, 24, .blockWidth = 4, .blockHeight = 4, .compressed = true, .numPlanes = 2), // Subsampled 420
    PROPS(YUV_420p, 24, .blockWidth = 4, .blockHeight = 4, .compressed = true, .numPlanes = 3), // Subsampled 420
};

//void TextureManager::initialize(const VulkanDevice& device, CommandManager& cmdManager, BufferManager& bufMgr){
//    vulkanDevice = &device;
//    commandManager = &cmdManager;
//    bufferManager = &bufMgr;
//}

void TextureManager::cleanup() {
    // Cleanup is handled by individual resource destruction
}

void TextureManager::createTexture(const TextureDesc& requestedDesc,
    const char* debugName,
    Result* outResult) {

    TextureDesc desc(requestedDesc);
    if (debugName && *debugName) {
        desc.debugName = debugName;
    }
    const VkFormat vkFormat =
        isDepthOrStencilFormat(desc.format) ?
        vulkanDevice_->getClosestDepthStencilFormat(desc.format) :
        formatToVkFormat(desc.format);
    const TextureType_e type = desc.type;
    vkUsageFlags_ =(desc.storage == StorageType_Device) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
    if (desc.usage & TextureUsageBits_Sampled) {
        vkUsageFlags_ |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (desc.usage & TextureUsageBits_Storage) {
        vkUsageFlags_ |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (desc.usage & TextureUsageBits_Attachment) {
        vkUsageFlags_ |= isDepthOrStencilFormat(desc.format) ?
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT :
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        if (desc.storage == StorageType_Memoryless) {
            vkUsageFlags_ |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        }
    }
    if (desc.storage != StorageType_Memoryless) {
        vkUsageFlags_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    };
    const VkMemoryPropertyFlags memFlags = storageTypeToVkMemoryPropertyFlags(desc.storage);
    const bool hasDebugName = desc.debugName && *desc.debugName;
    char debugNameImage[256] = { 0 };
    char debugNameImageView[256] = { 0 };
    if (hasDebugName) {
        snprintf(debugNameImage, sizeof(debugNameImage) - 1, "Image: %s", desc.debugName);
        snprintf(debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: %s", desc.debugName);
    }
    VkImageCreateFlags vkCreateFlags = 0;
    VkImageViewType vkImageViewType;
    vkSamples_ = VK_SAMPLE_COUNT_1_BIT;
    numLevels_ = desc.numLayers;
    switch (desc.type) {
    case TextureType_2D:
        vkImageViewType = numLayers_ > 1 ?
            VK_IMAGE_VIEW_TYPE_2D_ARRAY :
            VK_IMAGE_VIEW_TYPE_2D;
        vkType_ = VK_IMAGE_TYPE_2D;
        vkSamples_ = getVulkanSampleCountFlags(
            desc.numSamples, vulkanDevice_->getFramebufferMSAABitMask());
        break;
    case TextureType_3D:
        vkImageViewType = VK_IMAGE_VIEW_TYPE_3D;
        vkType_ = VK_IMAGE_TYPE_3D;
        break;
    case TextureType_Cube:
        vkImageViewType = numLayers_ > 1 ?
            VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        vkType_ = VK_IMAGE_TYPE_2D;
        vkCreateFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        numLayers_ *= 6;
        break;
    }
    const VkExtent3D vkExtent{
    desc.dimensions.width,
    desc.dimensions.height,
    desc.dimensions.depth };
    const uint32_t numLevels = desc.numMipLevels;  
    vkExtent_ = vkExtent;
    vkImageFormat_ = vkFormat;
    isDepthFormat_ = isDepthFormat(vkFormat);
    isStencilFormat_ = isStencilFormat(vkFormat);

    const VkImageCreateInfo ci = {
   .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
   .flags = vkCreateFlags,
   .imageType = vkType_,
   .format = vkFormat,
   .extent = vkExtent,
   .mipLevels = numLevels,
   .arrayLayers = numLayers_,
   .samples = vkSamples_,
   .tiling = VK_IMAGE_TILING_OPTIMAL,
   .usage = vkUsageFlags_,
   .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
   .queueFamilyIndexCount = 0,
   .pQueueFamilyIndices = nullptr,
   .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };

    const uint32_t numPlanes = 1; // simplified code path
    vkCreateImage(vulkanDevice_->getLogicalDevice(), &ci, nullptr, &vkImage_);
    constexpr uint32_t kNumMaxImagePlanes = 1;
    VkMemoryRequirements2 memRequirements[kNumMaxImagePlanes] = {
      {.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2 },
    };
    const VkImagePlaneMemoryRequirementsInfo
        planes[kNumMaxImagePlanes] = {
        {.sType = VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO,
          .planeAspect = VK_IMAGE_ASPECT_PLANE_0_BIT },
    };
    const VkImage img = vkImage_;
    const VkImageMemoryRequirementsInfo2
        imgRequirements[kNumMaxImagePlanes] = {
        {.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2,
          .pNext = numPlanes > 0 ? &planes[0] : nullptr,
          .image = img },
    };
    for (uint32_t p = 0; p != numPlanes; p++) {
        vkGetImageMemoryRequirements2(vulkanDevice_->getLogicalDevice(),
            &imgRequirements[p], &memRequirements[p]);
        allocateMemory(vulkanDevice_->getPhysicalDevice(), vulkanDevice_->getLogicalDevice(),
            &memRequirements[p], memFlags, &vkMemory_[p]);
    }
    const VkBindImagePlaneMemoryInfo
        bindImagePlaneMemoryInfo[kNumMaxImagePlanes] = {
        {.sType = VK_STRUCTURE_TYPE_BIND_IMAGE_PLANE_MEMORY_INFO,
          .planeAspect = VK_IMAGE_ASPECT_PLANE_0_BIT },
    };
    const VkBindImageMemoryInfo bindInfo[kNumMaxImagePlanes] = {
      getBindImageMemoryInfo(
        nullptr, img, vkMemory_[0]),
    };
    vkBindImageMemory2(vulkanDevice_->getLogicalDevice(), numPlanes, bindInfo);
    if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
        numPlanes == 1) {
        vkMapMemory(vulkanDevice_->getLogicalDevice(), vkMemory_[0], 0,
            VK_WHOLE_SIZE, 0, &mappedPtr_);
    }
    setDebugObjectName(vulkanDevice_->getLogicalDevice(), VK_OBJECT_TYPE_IMAGE,
        (uint64_t)vkImage_, debugNameImage);
    vkGetPhysicalDeviceFormatProperties(vulkanDevice_->getPhysicalDevice(),
        vkImageFormat_, &vkFormatProperties_);
    VkImageAspectFlags aspect = 0;
    if (isDepthFormat_ || isStencilFormat_) {
        if (isDepthFormat_) {
            aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else if (isStencilFormat_) {
            aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    const VkComponentMapping mapping = {
      .r = VkComponentSwizzle(desc.swizzle.r),
      .g = VkComponentSwizzle(desc.swizzle.g),
      .b = VkComponentSwizzle(desc.swizzle.b),
      .a = VkComponentSwizzle(desc.swizzle.a),
    };
    imageView_ = createImageView(
        vulkanDevice_->getLogicalDevice(), vkImageViewType, vkFormat, aspect, 0,
        VK_REMAINING_MIP_LEVELS, 0, numLayers_,
        mapping, nullptr, debugNameImageView);
    if (vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT) {
        if (!desc.identity()) {
            imageViewStorage_ = createImageView(
                vulkanDevice_->getLogicalDevice(), vkImageViewType, vkFormat, aspect, 0,
                VK_REMAINING_MIP_LEVELS, 0, numLayers_,
                {}, nullptr, debugNameImageView);
        }
    }
}

void TextureManager::createTextureView(const TextureViewDesc& desc,
    const char* debugName,
    Result* outResult) {
    isOwningVkImage_ = false;

    // drop all existing image views - they belong to the base image
    memset(&imageViewStorage_, 0, sizeof(imageViewStorage_));
    memset(&imageViewForFramebuffer_, 0, sizeof(imageViewForFramebuffer_));

    VkImageAspectFlags aspect = 0;
    if (isDepthFormat_ || isStencilFormat_) {
        if (isDepthFormat_) {
            aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else if (isStencilFormat_) {
            aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else {
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageViewType vkImageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    switch (desc.type) {
    case TextureType_2D:
        vkImageViewType = desc.numLayers > 1 ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        break;
    case TextureType_3D:
        vkImageViewType = VK_IMAGE_VIEW_TYPE_3D;
        break;
    case TextureType_Cube:
        vkImageViewType = desc.numLayers > 1 ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        break;
    default:
        VK_ASSERT_MSG(false, "Code should NOT be reached");
        Result::setResult(outResult, Result::Code::RuntimeError, "Unsupported texture view type");
        return;
    }

    const VkComponentMapping mapping = {
        .r = VkComponentSwizzle(desc.swizzle.r),
        .g = VkComponentSwizzle(desc.swizzle.g),
        .b = VkComponentSwizzle(desc.swizzle.b),
        .a = VkComponentSwizzle(desc.swizzle.a),
    };

    VK_ASSERT_MSG(getNumImagePlanes(vkImageFormat_) == 1, "Unsupported multiplanar image");

    imageView_ = createImageView(vulkanDevice_->getLogicalDevice(),
        vkImageViewType,
        vkImageFormat_,
        aspect,
        desc.mipLevel,
        desc.numMipLevels,
        desc.layer,
        desc.numLayers,
        mapping,
        nullptr,
        debugName);

    if (!VK_VERIFY(imageView_ != VK_NULL_HANDLE)) {
        Result::setResult(outResult, Result::Code::RuntimeError, "Cannot create VkImageView");
        return;
    }

    if (vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT) {
        if (!desc.identity()) {
            // use identity swizzle for storage images
            imageViewStorage_ = createImageView(vulkanDevice_->getLogicalDevice(),
                vkImageViewType,
                vkImageFormat_,
                aspect,
                desc.mipLevel,
                desc.numMipLevels,
                desc.layer,
                desc.numLayers,
                {},
                nullptr,
                debugName);
            VK_ASSERT(imageViewStorage_ != VK_NULL_HANDLE);
        }
    }
}

VkImageView TextureManager::createImageView(VkDevice device,
    VkImageViewType type,
    VkFormat format,
    VkImageAspectFlags aspectMask,
    uint32_t baseLevel,
    uint32_t numLevels,
    uint32_t baseLayer,
    uint32_t numLayers,
    const VkComponentMapping mapping,
    const VkSamplerYcbcrConversionInfo* ycbcr,
    const char* debugName) const {

    const VkImageViewCreateInfo ci = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = ycbcr,
    .image = vkImage_,
    .viewType = type,
    .format = format,
    .components = mapping,
    .subresourceRange = { aspectMask, baseLevel,
      numLevels ? numLevels : numLevels_, baseLayer, numLayers},
    };
    VkImageView vkView = VK_NULL_HANDLE;
    vkCreateImageView(device, &ci, nullptr, &vkView);
    setDebugObjectName(device, VK_OBJECT_TYPE_IMAGE_VIEW, (uint64_t)vkView, debugName);
    return vkView;
}

void TextureManager::transitionLayout(VkCommandBuffer commandBuffer,
    VkImageLayout newImageLayout,
    const VkImageSubresourceRange& subresourceRange) const {

    const VkImageLayout oldImageLayout =
        vkImageLayout_ == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
        ? (isDepthAttachment() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
        : vkImageLayout_;

    if (newImageLayout == VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL) {
        newImageLayout = isDepthAttachment() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    StageAccess src = getPipelineStageAccess(oldImageLayout);
    StageAccess dst = getPipelineStageAccess(newImageLayout);

    if (isDepthAttachment()) {
        src.stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        dst.stage |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
        src.access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
        dst.access |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
    }

    const VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = src.stage,
        .srcAccessMask = src.access,
        .dstStageMask = dst.stage,
        .dstAccessMask = dst.access,
        .oldLayout = vkImageLayout_,
        .newLayout = newImageLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = vkImage_,
        .subresourceRange = subresourceRange,
    };

    const VkDependencyInfo depInfo{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(commandBuffer, &depInfo);

    vkImageLayout_ = newImageLayout;
}

uint32_t TextureManager::getTextureBytesPerLayer(uint32_t width, uint32_t height, Format_e format, uint32_t level) {
    const uint32_t levelWidth = std::max(width >> level, 1u);
    const uint32_t levelHeight = std::max(height >> level, 1u);

    const TextureFormatProperties props = properties[format];

    if (!props.compressed) {
        return props.bytesPerBlock * levelWidth * levelHeight;
    }

    const uint32_t blockWidth = std::max((uint32_t)props.blockWidth, 1u);
    const uint32_t blockHeight = std::max((uint32_t)props.blockHeight, 1u);
    const uint32_t widthInBlocks = (levelWidth + props.blockWidth - 1) / props.blockWidth;
    const uint32_t heightInBlocks = (levelHeight + props.blockHeight - 1) / props.blockHeight;
    return widthInBlocks * heightInBlocks * props.bytesPerBlock;
}

VkImageAspectFlags TextureManager::getImageAspectFlags() const {
    VkImageAspectFlags flags = 0;

    flags |= isDepthFormat_ ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
    flags |= isStencilFormat_ ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
    flags |= !(isDepthFormat_ || isStencilFormat_) ? VK_IMAGE_ASPECT_COLOR_BIT : 0;

    return flags;
}

VkImageView TextureManager::getOrCreateVkImageViewForFramebuffer(VulkanEngine& eng, uint8_t level, uint16_t layer) {
    VK_ASSERT(level < VK_MAX_MIP_LEVELS);
    VK_ASSERT(layer < VK_UTILS_GET_ARRAY_SIZE(imageViewForFramebuffer_[0]));

    if (level >= VK_MAX_MIP_LEVELS || layer >= VK_UTILS_GET_ARRAY_SIZE(imageViewForFramebuffer_[0])) {
        return VK_NULL_HANDLE;
    }

    if (imageViewForFramebuffer_[level][layer] != VK_NULL_HANDLE) {
        return imageViewForFramebuffer_[level][layer];
    }

    char debugNameImageView[320] = { 0 };
    snprintf(
        debugNameImageView, sizeof(debugNameImageView) - 1, "Image View: '%s' imageViewForFramebuffer_[%u][%u]", ":)", level, layer);

    imageViewForFramebuffer_[level][layer] = createImageView(eng.vulkanDevice_.get()->getLogicalDevice(),
        VK_IMAGE_VIEW_TYPE_2D,
        vkImageFormat_,
        getImageAspectFlags(),
        level,
        1u,
        layer,
        1u,
        {},
        nullptr,
        debugNameImageView);

    return imageViewForFramebuffer_[level][layer];
}
//
//void TextureManager::createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
//                    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
//                    VkImage& image, VkDeviceMemory& imageMemory) {
//    VkImageCreateInfo imageInfo{};
//    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//    imageInfo.imageType = VK_IMAGE_TYPE_2D;
//    imageInfo.extent.width = width;
//    imageInfo.extent.height = height;
//    imageInfo.extent.depth = 1;
//    imageInfo.mipLevels = 1;
//    imageInfo.arrayLayers = 1;
//    imageInfo.format = format;
//    imageInfo.tiling = tiling;
//    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    imageInfo.usage = usage;
//    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
//    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//    if (vkCreateImage(vulkanDevice->getLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create image!");
//    }
//
//    VkMemoryRequirements memRequirements;
//    vkGetImageMemoryRequirements(vulkanDevice->getLogicalDevice(), image, &memRequirements);
//
//    VkMemoryAllocateInfo allocInfo{};
//    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//    allocInfo.allocationSize = memRequirements.size;
//    allocInfo.memoryTypeIndex = vulkanDevice->findMemoryType(memRequirements.memoryTypeBits, properties);
//
//    if (vkAllocateMemory(vulkanDevice->getLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
//        throw std::runtime_error("failed to allocate image memory!");
//    }
//
//    vkBindImageMemory(vulkanDevice->getLogicalDevice(), image, imageMemory, 0);
//}
//
//VkImageView TextureManager::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
//    VkImageViewCreateInfo viewInfo{};
//    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//    viewInfo.image = image;
//    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
//    viewInfo.format = format;
//    viewInfo.subresourceRange.aspectMask = aspectFlags;
//    viewInfo.subresourceRange.baseMipLevel = 0;
//    viewInfo.subresourceRange.levelCount = 1;
//    viewInfo.subresourceRange.baseArrayLayer = 0;
//    viewInfo.subresourceRange.layerCount = 1;
//
//    VkImageView imageView;
//    if (vkCreateImageView(vulkanDevice->getLogicalDevice(), &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create image view!");
//    }
//
//    return imageView;
//}

//void TextureManager::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
//    VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();
//
//    VkImageMemoryBarrier barrier{};
//    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//    barrier.oldLayout = oldLayout;
//    barrier.newLayout = newLayout;
//    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//    barrier.image = image;
//    barrier.subresourceRange.baseMipLevel = 0;
//    barrier.subresourceRange.levelCount = 1;
//    barrier.subresourceRange.baseArrayLayer = 0;
//    barrier.subresourceRange.layerCount = 1;
//
//    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
//        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
//        if (hasStencilComponent(format)) {
//            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
//        }
//    } else {
//        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    }
//
//    VkPipelineStageFlags sourceStage;
//    VkPipelineStageFlags destinationStage;
//
//    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
//        barrier.srcAccessMask = 0;
//        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//
//        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
//    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
//        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
//        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
//        barrier.srcAccessMask = 0;
//        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//
//        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//    } else {
//        throw std::invalid_argument("unsupported layout transition!");
//    }
//
//    vkCmdPipelineBarrier(
//        commandBuffer,
//        sourceStage, destinationStage,
//        0,
//        0, nullptr,
//        0, nullptr,
//        1, &barrier
//    );
//
//    commandManager->endSingleTimeCommands(commandBuffer);
//}

//void TextureManager::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
//    VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();
//
//    VkBufferImageCopy region{};
//    region.bufferOffset = 0;
//    region.bufferRowLength = 0;
//    region.bufferImageHeight = 0;
//
//    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    region.imageSubresource.mipLevel = 0;
//    region.imageSubresource.baseArrayLayer = 0;
//    region.imageSubresource.layerCount = 1;
//
//    region.imageOffset = {0, 0, 0};
//    region.imageExtent = {
//        width,
//        height,
//        1
//    };
//
//    vkCmdCopyBufferToImage(
//        commandBuffer,
//        buffer,
//        image,
//        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//        1,
//        &region
//    );
//
//    commandManager->endSingleTimeCommands(commandBuffer);
//}

void TextureManager::createTextureFromFile(const std::string& texturePath, VkImage& textureImage, 
                              VkDeviceMemory& textureImageMemory, VkImageView& textureImageView){
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }
    /*uint32_t numMipLevels = 1;

    while ((texWidth | texHeight) >> numMipLevels) {
        numMipLevels++;
    }


    ktxTextureCreateInfo createInfo = {
        .glInternalformat = GL_COMPRESSED_RGBA_BPTC_UNORM,
        .vkFormat = VK_FORMAT_BC7_UNORM_BLOCK,
        .baseWidth = (uint32_t)texWidth,
        .baseHeight = (uint32_t)texHeight,
        .baseDepth = 1u,
        .numDimensions = 2u,
        .numLevels = numMipLevels,
        .numLayers = 1u,
        .numFaces = 1u,
        .generateMipmaps = KTX_FALSE,
    };
    ktxTexture2* textureKTX2 = nullptr;
    ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureKTX2);
    int w = texWidth;
    int h = texHeight;
    for (uint32_t i = 0; i != numMipLevels; i++) {
        size_t offset = 0;
        ktxTexture_GetImageOffset(
            ktxTexture(textureKTX2), i, 0, 0, &offset
        );
        stbir_resize_uint8_linear(
            (const unsigned char*)pixels, texWidth, texHeight, 0,
            ktxTexture_GetData(ktxTexture(textureKTX2)) + offset,
            w, h, 0, STBIR_RGBA
        );
        h = h > 1 ? h >> 1 : 1;
        w = w > 1 ? w >> 1 : 1;
    }
    ktxTexture2_CompressBasis(textureKTX2, 255);
    ktxTexture2_TranscodeBasis(textureKTX2, KTX_TTF_BC7_RGBA, 0);

    ktxTexture1* textureKTX1 = nullptr;
    ktxTexture1_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &textureKTX1);
    
    for (uint32_t i = 0; i != numMipLevels; i++) {
        size_t offset1 = 0;
        ktxTexture_GetImageOffset(
            ktxTexture(textureKTX1), i, 0, 0, &offset1
        );
        size_t offset2 = 0;
        ktxTexture_GetImageOffset(
            ktxTexture(textureKTX2), i, 0, 0, &offset2
        );
        memcpy(
            ktxTexture_GetData(ktxTexture(textureKTX1)) + offset1,
            ktxTexture_GetData(ktxTexture(textureKTX2)) + offset2,
            ktxTexture_GetImageSize(ktxTexture(textureKTX1), i)
        );
        ktxTexture_WriteToNamedFile(ktxTexture(textureKTX1), ".cache/image.ktx");
        ktxTexture_Destroy(ktxTexture(textureKTX1));
        ktxTexture_Destroy(ktxTexture(textureKTX2));
    }*/

  /*  VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    bufferManager->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
    void* data;
    vkMapMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(texWidth, texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(vulkanDevice->getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory, nullptr);

    textureImageView = createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);*/
}

/*VkSampler TextureManager::createTextureSampler(){
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(vulkanDevice->getPhysicalDevice(), &properties);

    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler sampler;
    if (vkCreateSampler(vulkanDevice->getLogicalDevice(), &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }

    return sampler;
}*/   

//void TextureManager::createDepthResources(VkExtent2D extent, VkImage& depthImage, 
//                                         VkDeviceMemory& depthImageMemory, VkImageView& depthImageView) {
//    VkFormat depthFormat = findDepthFormat();
//    createImage(extent.width, extent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, 
//               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
//               depthImage, depthImageMemory);
//    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
//    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
//}
//
//VkFormat TextureManager::findDepthFormat() {
//    return findSupportedFormat(
//        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
//        VK_IMAGE_TILING_OPTIMAL,
//        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
//    );
//}
//
//VkFormat TextureManager::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
//    for (VkFormat format : candidates) {
//        VkFormatProperties props;
//        vkGetPhysicalDeviceFormatProperties(vulkanDevice->getPhysicalDevice(), format, &props);
//
//        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
//            return format;
//        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
//            return format;
//        }
//    }
//
//    throw std::runtime_error("failed to find supported format!");
//}

//bool TextureManager::hasStencilComponent(VkFormat format) {
//    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
//}

bool TextureManager::isDepthOrStencilFormat(Format_e format) {
    return properties[format].depth || properties[format].stencil;
}

//void TextureManager::destroyImage(VkImage& image, VkDeviceMemory& imageMemory) {
//    if (image != VK_NULL_HANDLE) {
//        vkDestroyImage(vulkanDevice->getLogicalDevice(), image, nullptr);
//        image = VK_NULL_HANDLE;
//    }
//    if (imageMemory != VK_NULL_HANDLE) {
//        vkFreeMemory(vulkanDevice->getLogicalDevice(), imageMemory, nullptr);
//        imageMemory = VK_NULL_HANDLE;
//    }
//}
//
//void TextureManager::destroyImageView(VkImageView& imageView) {
//    if (imageView != VK_NULL_HANDLE) {
//        vkDestroyImageView(vulkanDevice->getLogicalDevice(), imageView, nullptr);
//        imageView = VK_NULL_HANDLE;
//    }
//}
//
//void TextureManager::destroySampler(VkSampler& sampler) {
//    if (sampler != VK_NULL_HANDLE) {
//        vkDestroySampler(vulkanDevice->getLogicalDevice(), sampler, nullptr);
//        sampler = VK_NULL_HANDLE;
//    }
//}

void transitionToColorAttachment(VkCommandBuffer buffer, TextureManager* colorTex) {
    if (!VK_VERIFY(colorTex)) {
        return;
    }

    if (!VK_VERIFY(!colorTex->getIsDepthFormat() && !colorTex->getIsStencilFormat())) {
        VK_ASSERT_MSG(false, "Color attachments cannot have depth/stencil formats");
        return;
    }
    VK_ASSERT_MSG(colorTex->getImageFormat() != VK_FORMAT_UNDEFINED, "Invalid color attachment format");
    colorTex->transitionLayout(buffer,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
}

