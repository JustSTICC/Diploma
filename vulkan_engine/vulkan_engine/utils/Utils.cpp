#include "Utils.h"
#include "../validation/VulkanValidator.h"


void imageMemoryBarrier(VkCommandBuffer buffer,
    VkImage image,
    StageAccess src,
    StageAccess dst,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkImageSubresourceRange subresourceRange) {
    const VkImageMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = src.stage,
        .srcAccessMask = src.access,
        .dstStageMask = dst.stage,
        .dstAccessMask = dst.access,
        .oldLayout = oldImageLayout,
        .newLayout = newImageLayout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange = subresourceRange,
    };

    const VkDependencyInfo depInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(buffer, &depInfo);
}

VkFilter samplerFilterToVkFilter(SamplerFilter_e filter) {
    switch (filter) {
    case SamplerFilter_Nearest:
        return VK_FILTER_NEAREST;
    case SamplerFilter_Linear:
        return VK_FILTER_LINEAR;
    }
    VK_ASSERT_MSG(false, "SamplerFilter value not handled: %d", (int)filter);
    return VK_FILTER_LINEAR;
}

VkSamplerMipmapMode samplerMipMapToVkSamplerMipmapMode(SamplerMip_e filter) {
    switch (filter) {
    case SamplerMip_Disabled:
    case SamplerMip_Nearest:
        return VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case SamplerMip_Linear:
        return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
    VK_ASSERT_MSG(false, "SamplerMipMap value not handled: %d", (int)filter);
    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
}

VkSamplerAddressMode samplerWrapModeToVkSamplerAddressMode(SamplerWrap_e mode) {
    switch (mode) {
    case SamplerWrap_Repeat:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case SamplerWrap_Clamp:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case SamplerWrap_MirrorRepeat:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    }
    VK_ASSERT_MSG(false, "SamplerWrapMode value not handled: %d", (int)mode);
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

VkSamplerCreateInfo samplerStateDescToVkSamplerCreateInfo(const SamplerStateDesc& desc, const VkPhysicalDeviceLimits& limits) {
    VK_ASSERT_MSG(desc.mipLodMax >= desc.mipLodMin,
        "mipLodMax (%d) must be greater than or equal to mipLodMin (%d)",
        (int)desc.mipLodMax,
        (int)desc.mipLodMin);

    VkSamplerCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .magFilter = samplerFilterToVkFilter(desc.magFilter),
        .minFilter = samplerFilterToVkFilter(desc.minFilter),
        .mipmapMode = samplerMipMapToVkSamplerMipmapMode(desc.mipMap),
        .addressModeU = samplerWrapModeToVkSamplerAddressMode(desc.wrapU),
        .addressModeV = samplerWrapModeToVkSamplerAddressMode(desc.wrapV),
        .addressModeW = samplerWrapModeToVkSamplerAddressMode(desc.wrapW),
        .mipLodBias = 0.0f,
        .anisotropyEnable = VK_FALSE,
        .maxAnisotropy = 0.0f,
        .compareEnable = desc.depthCompareEnabled ? VK_TRUE : VK_FALSE,
        .compareOp = desc.depthCompareEnabled ? desc.depthCompareOp : VK_COMPARE_OP_ALWAYS,
        .minLod = float(desc.mipLodMin),
        .maxLod = desc.mipMap == SamplerMip_Disabled ? float(desc.mipLodMin) : float(desc.mipLodMax),
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
    };
	return ci;
}


VkAttachmentLoadOp loadOpToVkAttachmentLoadOp(LoadOp_e a) {
    switch (a) {
    case LoadOp_Invalid:
        VK_ASSERT(false);
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case LoadOp_DontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case LoadOp_Load:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case LoadOp_Clear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case LoadOp_None:
        return VK_ATTACHMENT_LOAD_OP_NONE_EXT;
    }
    VK_ASSERT(false);
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
}

VkAttachmentStoreOp storeOpToVkAttachmentStoreOp(StoreOp_e a) {
    switch (a) {
    case StoreOp_DontCare:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    case StoreOp_Store:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case StoreOp_MsaaResolve:
        // for MSAA resolve, we have to store data into a special "resolve" attachment
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    case StoreOp_None:
        return VK_ATTACHMENT_STORE_OP_NONE;
    }
    VK_ASSERT(false);
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
}

VkIndexType indexFormatToVkIndexType(IndexFormat_e fmt) {
    switch (fmt) {
    case IndexFormat_UI8:
        return VK_INDEX_TYPE_UINT8_EXT;
    case IndexFormat_UI16:
        return VK_INDEX_TYPE_UINT16;
    case IndexFormat_UI32:
        return VK_INDEX_TYPE_UINT32;
    };
    VK_ASSERT(false);
    return VK_INDEX_TYPE_NONE_KHR;
}

VkMemoryPropertyFlags storageTypeToVkMemoryPropertyFlags(StorageType_e storage) {
    VkMemoryPropertyFlags memFlags{ 0 };

    switch (storage) {
    case StorageType_Device:
        memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
    case StorageType_HostVisible:
        memFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    case StorageType_Memoryless:
        memFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        break;
    }
    return memFlags;
}

uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t memoryTypeBits, VkMemoryPropertyFlags flags) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physDev, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        const bool hasProperties = (memProperties.memoryTypes[i].propertyFlags & flags) == flags;
        if ((memoryTypeBits & (1 << i)) && hasProperties) {
            return i;
        }
    }
    assert(false);

    return 0;
}

VkResult allocateMemory(VkPhysicalDevice physDev,
    VkDevice device,
    const VkMemoryRequirements2* memRequirements,
    VkMemoryPropertyFlags props,
    VkDeviceMemory* outMemory) {
    assert(memRequirements);

    const VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
    };
    const VkMemoryAllocateInfo ai = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &memoryAllocateFlagsInfo,
        .allocationSize = memRequirements->memoryRequirements.size,
        .memoryTypeIndex = findMemoryType(physDev, memRequirements->memoryRequirements.memoryTypeBits, props),
    };
    return vkAllocateMemory(device, &ai, NULL, outMemory);
}

VkBindImageMemoryInfo getBindImageMemoryInfo(const VkBindImagePlaneMemoryInfo* next, VkImage image, VkDeviceMemory memory) {
    return VkBindImageMemoryInfo{
        .sType = VK_STRUCTURE_TYPE_BIND_IMAGE_MEMORY_INFO,
        .pNext = next,
        .image = image,
        .memory = memory,
        .memoryOffset = 0,
    };
}

std::vector<VkFormat> getCompatibleDepthStencilFormats(Format_e format) {
    switch (format) {
    case Format_Z_UN16:
        return { VK_FORMAT_D16_UNORM, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT };
    case Format_Z_UN24:
        return { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM_S8_UINT };
    case Format_Z_F32:
        return { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT };
    case Format_Z_UN24_S_UI8:
        return { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT };
    case Format_Z_F32_S_UI8:
        return { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT };
    default:
        return { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT };
    }
    return { VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT };
}

VkFormat formatToVkFormat(Format_e format) {
    using TextureFormat = ::Format_e;
    switch (format) {
    case Format_Invalid:
        return VK_FORMAT_UNDEFINED;
    case Format_R_UN8:
        return VK_FORMAT_R8_UNORM;
    case Format_R_UN16:
        return VK_FORMAT_R16_UNORM;
    case Format_R_F16:
        return VK_FORMAT_R16_SFLOAT;
    case Format_R_UI16:
        return VK_FORMAT_R16_UINT;
    case Format_R_UI32:
        return VK_FORMAT_R32_UINT;
    case Format_RG_UN8:
        return VK_FORMAT_R8G8_UNORM;
    case Format_RG_UI16:
        return VK_FORMAT_R16G16_UINT;
    case Format_RG_UI32:
        return VK_FORMAT_R32G32_UINT;
    case Format_RG_UN16:
        return VK_FORMAT_R16G16_UNORM;
    case Format_BGRA_UN8:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case Format_RGBA_UN8:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case Format_RGBA_SRGB8:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case Format_BGRA_SRGB8:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case Format_RG_F16:
        return VK_FORMAT_R16G16_SFLOAT;
    case Format_RG_F32:
        return VK_FORMAT_R32G32_SFLOAT;
    case Format_R_F32:
        return VK_FORMAT_R32_SFLOAT;
    case Format_RGBA_F16:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
    case Format_RGBA_UI32:
        return VK_FORMAT_R32G32B32A32_UINT;
    case Format_RGBA_F32:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case Format_A2B10G10R10_UN:
        return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
    case Format_A2R10G10B10_UN:
        return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
    case Format_ETC2_RGB8:
        return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case Format_ETC2_SRGB8:
        return VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
    case Format_BC7_RGBA:
        return VK_FORMAT_BC7_UNORM_BLOCK;
    case Format_Z_UN16:
        return VK_FORMAT_D16_UNORM;
    case Format_Z_UN24:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format_Z_F32:
        return VK_FORMAT_D32_SFLOAT;
    case Format_Z_UN24_S_UI8:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case Format_Z_F32_S_UI8:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    case Format_YUV_NV12:
        return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
    case Format_YUV_420p:
        return VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    }
}

Format_e vkFormatToFormat(VkFormat format) {
    switch (format) {
    case VK_FORMAT_UNDEFINED:
        return Format_Invalid;
    case VK_FORMAT_R8_UNORM:
        return Format_R_UN8;
    case VK_FORMAT_R16_UNORM:
        return Format_R_UN16;
    case VK_FORMAT_R16_SFLOAT:
        return Format_R_F16;
    case VK_FORMAT_R16_UINT:
        return Format_R_UI16;
    case VK_FORMAT_R8G8_UNORM:
        return Format_RG_UN8;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return Format_BGRA_UN8;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return Format_RGBA_UN8;
    case VK_FORMAT_R8G8B8A8_SRGB:
        return Format_RGBA_SRGB8;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return Format_BGRA_SRGB8;
    case VK_FORMAT_R16G16_UNORM:
        return Format_RG_UN16;
    case VK_FORMAT_R16G16_SFLOAT:
        return Format_RG_F16;
    case VK_FORMAT_R32G32_SFLOAT:
        return Format_RG_F32;
    case VK_FORMAT_R16G16_UINT:
        return Format_RG_UI16;
    case VK_FORMAT_R32_SFLOAT:
        return Format_R_F32;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return Format_RGBA_F16;
    case VK_FORMAT_R32G32B32A32_UINT:
        return Format_RGBA_UI32;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return Format_RGBA_F32;
    case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
        return Format_A2B10G10R10_UN;
    case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
        return Format_A2R10G10B10_UN;
    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK:
        return Format_ETC2_RGB8;
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK:
        return Format_ETC2_SRGB8;
    case VK_FORMAT_D16_UNORM:
        return Format_Z_UN16;
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return Format_BC7_RGBA;
    case VK_FORMAT_X8_D24_UNORM_PACK32:
        return Format_Z_UN24;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return Format_Z_UN24_S_UI8;
    case VK_FORMAT_D32_SFLOAT:
        return Format_Z_F32;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return Format_Z_F32_S_UI8;
    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
        return Format_YUV_NV12;
    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
        return Format_YUV_420p;
    default:;
    }
    VK_ASSERT_MSG(false, "VkFormat value not handled: %d", (int)format);
    return Format_Invalid;
}

uint32_t getNumImagePlanes(VkFormat format) {
    switch (format) {
    case VK_FORMAT_UNDEFINED:
        return 0;
    case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM:
    case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM:
    case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM:
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16:
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16:
    case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16:
    case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM:
    case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM:
    case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM:
        return 3;
    case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM:
    case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM:
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16:
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16:
    case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM:
    case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM:
    case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM:
    case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16:
    case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16:
    case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM:
        return 2;
    default:
        return 1;
    }
}


VkSampleCountFlagBits getVulkanSampleCountFlags(uint32_t numSamples, VkSampleCountFlags maxSamplesMask) {
    if (numSamples <= 1 || VK_SAMPLE_COUNT_2_BIT > maxSamplesMask) {
        return VK_SAMPLE_COUNT_1_BIT;
    }
    if (numSamples <= 2 || VK_SAMPLE_COUNT_4_BIT > maxSamplesMask) {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    if (numSamples <= 4 || VK_SAMPLE_COUNT_8_BIT > maxSamplesMask) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (numSamples <= 8 || VK_SAMPLE_COUNT_16_BIT > maxSamplesMask) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (numSamples <= 16 || VK_SAMPLE_COUNT_32_BIT > maxSamplesMask) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (numSamples <= 32 || VK_SAMPLE_COUNT_64_BIT > maxSamplesMask) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    return VK_SAMPLE_COUNT_64_BIT;
}

uint32_t getBytesPerPixel(VkFormat format) {
    switch (format) {
    case VK_FORMAT_R8_UNORM:
        return 1;
    case VK_FORMAT_R16_SFLOAT:
        return 2;
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_B8G8R8_UNORM:
        return 3;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_B8G8R8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SRGB:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R32_SFLOAT:
    case VK_FORMAT_R32_UINT:
        return 4;
    case VK_FORMAT_R16G16B16_SFLOAT:
        return 6;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
    case VK_FORMAT_R32G32_UINT:
        return 8;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 12;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 16;
    default:;
    }
    VK_ASSERT_MSG(false, "VkFormat value not handled: %d", (int)format);
    return 1;
}

StageAccess getPipelineStageAccess(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return {
            .stage = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT,
            .access = VK_ACCESS_2_NONE,
        };
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            .access = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        };
    case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT,
            .access = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        };
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
                     VK_PIPELINE_STAGE_2_PRE_RASTERIZATION_SHADERS_BIT,
            .access = VK_ACCESS_2_SHADER_READ_BIT,
        };
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .access = VK_ACCESS_2_TRANSFER_READ_BIT,
        };
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .access = VK_ACCESS_2_TRANSFER_WRITE_BIT,
        };
    case VK_IMAGE_LAYOUT_GENERAL:
        return {
            .stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
        };
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return {
            .stage = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .access = VK_ACCESS_2_NONE | VK_ACCESS_2_SHADER_WRITE_BIT,
        };
    default:
        VK_ASSERT_MSG(false, "Unsupported image layout transition!");
        return {
            .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
            .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT,
        };
    }
};

bool isDepthFormat(VkFormat format) {
    return (format == VK_FORMAT_D16_UNORM) || (format == VK_FORMAT_X8_D24_UNORM_PACK32) || (format == VK_FORMAT_D32_SFLOAT) ||
        (format == VK_FORMAT_D16_UNORM_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT) || (format == VK_FORMAT_D32_SFLOAT_S8_UINT);
}

bool isStencilFormat(VkFormat format) {
    return (format == VK_FORMAT_S8_UINT) || (format == VK_FORMAT_D16_UNORM_S8_UINT) || (format == VK_FORMAT_D24_UNORM_S8_UINT) ||
        (format == VK_FORMAT_D32_SFLOAT_S8_UINT);
}

VkDeviceSize getAlignedSize(uint64_t value, uint64_t alignment) {
    return (value + alignment - 1) & ~(alignment - 1);
}

VkExtent2D getImagePlaneExtent(VkExtent2D plane0, Format_e format, uint32_t plane) {
    switch (format) {
    case Format_YUV_NV12:
        return VkExtent2D{
            .width = plane0.width >> plane,
            .height = plane0.height >> plane,
        };
    case Format_YUV_420p:
        return VkExtent2D{
            .width = plane0.width >> (plane ? 1 : 0),
            .height = plane0.height >> (plane ? 1 : 0),
        };
    default:;
    }
    return plane0;
}

Result validateRange(const VkExtent3D& ext, uint32_t numLevels, const TextureRangeDesc& range) {
    if (!VK_VERIFY(range.dimensions.width > 0 && range.dimensions.height > 0 || range.dimensions.depth > 0 || range.numLayers > 0 ||
        range.numMipLevels > 0)) {
        return Result{ Result::Code::ArgumentOutOfRange, "width, height, depth numLayers, and numMipLevels must be > 0" };
    }
    if (range.mipLevel > numLevels) {
        return Result{ Result::Code::ArgumentOutOfRange, "range.mipLevel exceeds texture mip-levels" };
    }

    const uint32_t texWidth = std::max(ext.width >> range.mipLevel, 1u);
    const uint32_t texHeight = std::max(ext.height >> range.mipLevel, 1u);
    const uint32_t texDepth = std::max(ext.depth >> range.mipLevel, 1u);

    if (range.dimensions.width > texWidth || range.dimensions.height > texHeight || range.dimensions.depth > texDepth) {
        return Result{ Result::Code::ArgumentOutOfRange, "range dimensions exceed texture dimensions" };
    }
    if (range.offset.x > texWidth - range.dimensions.width || range.offset.y > texHeight - range.dimensions.height ||
        range.offset.z > texDepth - range.dimensions.depth) {
        return Result{ Result::Code::ArgumentOutOfRange, "range dimensions exceed texture dimensions" };
    }

    return Result{};
}


