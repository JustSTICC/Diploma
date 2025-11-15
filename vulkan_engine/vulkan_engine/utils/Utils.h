#pragma once
#include <string>
//#include <glslang/Include/glslang_c_interface.h>
#include <vulkan/vulkan.hpp>
#include "../common/render_def.h"

#define VK_UTILS_GET_ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

struct StageAccess {
	VkPipelineStageFlags2 stage;
	VkAccessFlags2 access;
};


void imageMemoryBarrier(VkCommandBuffer buffer,
	VkImage image,
	StageAccess src,
	StageAccess dst,
	VkImageLayout oldImageLayout,
	VkImageLayout newImageLayout,
	VkImageSubresourceRange subresourceRange);

StageAccess getPipelineStageAccess(VkImageLayout layout);

VkFilter samplerFilterToVkFilter(SamplerFilter_e filter);

VkSamplerMipmapMode samplerMipMapToVkSamplerMipmapMode(SamplerMip_e filter);

VkSamplerAddressMode samplerWrapModeToVkSamplerAddressMode(SamplerWrap_e mode);

VkSamplerCreateInfo samplerStateDescToVkSamplerCreateInfo(const SamplerStateDesc& desc, const VkPhysicalDeviceLimits& limits);

VkAttachmentLoadOp loadOpToVkAttachmentLoadOp(LoadOp_e a);

VkAttachmentStoreOp storeOpToVkAttachmentStoreOp(StoreOp_e a);

VkIndexType indexFormatToVkIndexType(IndexFormat_e fmt);

uint32_t findMemoryType(VkPhysicalDevice physDev, uint32_t memoryTypeBits, VkMemoryPropertyFlags flags);

VkMemoryPropertyFlags storageTypeToVkMemoryPropertyFlags(StorageType_e storage);

VkResult allocateMemory(VkPhysicalDevice physDev,
	VkDevice device,
	const VkMemoryRequirements2* memRequirements,
	VkMemoryPropertyFlags props,
	VkDeviceMemory* outMemory);

VkBindImageMemoryInfo getBindImageMemoryInfo(const VkBindImagePlaneMemoryInfo* next,
	VkImage image,
	VkDeviceMemory memory);

std::vector<VkFormat> getCompatibleDepthStencilFormats(Format_e format);

VkFormat formatToVkFormat(Format_e format);

uint32_t getBytesPerPixel(VkFormat format);

Format_e vkFormatToFormat(VkFormat format);

uint32_t getNumImagePlanes(VkFormat format);

VkSampleCountFlagBits getVulkanSampleCountFlags(uint32_t numSamples, VkSampleCountFlags maxSamplesMask);

bool isDepthFormat(VkFormat format);

bool isStencilFormat(VkFormat format);

VkDeviceSize getAlignedSize(uint64_t value, uint64_t alignment);

VkExtent2D getImagePlaneExtent(VkExtent2D plane0, Format_e format, uint32_t plane);

Result validateRange(const VkExtent3D& ext, uint32_t numLevels, const TextureRangeDesc& range);