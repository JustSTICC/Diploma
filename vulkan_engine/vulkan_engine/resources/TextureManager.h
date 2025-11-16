#pragma once

#include "../common/render_def.h"
//#include "../core/VulkanDevice.h"
//#include "../rendering/CommandManager.h"
//#include "../resources/BufferManager.h"
class VulkanEngine;
class VulkanDevice;

class TextureManager{
    public:

		TextureManager() = default;
        TextureManager(VulkanDevice* vulkanDevice);
        ~TextureManager();

        //void initialize(const VulkanDevice& device, CommandManager& commandManager, BufferManager& bufferManager);
        void cleanup();

        void createTexture(const TextureDesc& requestedDesc,
            const char* debugName,
            Result* outResult);
        void createTextureView(const TextureViewDesc& desc,
            const char* debugName,
			Result* outResult);

        const void* loadFromFile(const char* imagePath);
		void releseImgData();
        /*void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, 
                    VkImageUsageFlags usage, VkMemoryPropertyFlags properties, 
                    VkImage& image, VkDeviceMemory& imageMemory);*/

        //VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
        void transitionLayout(VkCommandBuffer commandBuffer,
            VkImageLayout newImageLayout,
            const VkImageSubresourceRange& subresourceRange) const;
        //void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
        //void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

        /*void createTextureFromFile(const std::string& texturePath, VkImage& textureImage, 
                              VkDeviceMemory& textureImageMemory, VkImageView& textureImageView);*/
        //VkSampler createTextureSampler();

        //void createDepthResources(VkExtent2D extent, VkImage& depthImage, 
        //                     VkDeviceMemory& depthImageMemory, VkImageView& depthImageView);

        //void destroyImage(VkImage& image, VkDeviceMemory& imageMemory);
        //void destroyImageView(VkImageView& imageView);
        //void destroySampler(VkSampler& sampler);

        VkImageView createImageView(
            VkDevice device,
            VkImageViewType type,
            VkFormat format,
            VkImageAspectFlags aspectMask,
            uint32_t baseLevel,
            uint32_t numLevels = VK_REMAINING_MIP_LEVELS,
            uint32_t baseLayer = 0,

            uint32_t numLayers = 1,
            const VkComponentMapping mapping = {
              .r = VK_COMPONENT_SWIZZLE_IDENTITY,
              .g = VK_COMPONENT_SWIZZLE_IDENTITY,
              .b = VK_COMPONENT_SWIZZLE_IDENTITY,
              .a = VK_COMPONENT_SWIZZLE_IDENTITY },
              const VkSamplerYcbcrConversionInfo* ycbcr = nullptr,
              const char* debugName = nullptr) const;
        VkImageView getOrCreateVkImageViewForFramebuffer(VulkanEngine& eng, uint8_t level, uint16_t layer);

        static uint32_t getTextureBytesPerLayer(uint32_t width, uint32_t height, Format_e format, uint32_t levelm);
        VkImageAspectFlags getImageAspectFlags() const;

        void* mappedPtr_ = nullptr;
        VkImageView imageViewForFramebuffer_[VK_MAX_MIP_LEVELS][6] = {};

		void setCurrentLayout(VkImageLayout layout) const { vkImageLayout_ = layout; }
        
		uint8_t* getImageData() const { return (uint8_t*)img_; }
		VkDeviceMemory getVkMemory() const { return vkMemory_[0]; }
        VkImage getVkImage() const { return vkImage_; }
		VkImageView getVkImageView() const { return imageView_; }
		VkImageView getVkImageViewStorage() const { return imageViewStorage_; }
		VkImageLayout getCurrentLayout() const { return vkImageLayout_; }
		VkImageType getImageType() const { return vkType_; }
        VkExtent3D getExtent() const { return vkExtent_; }
		VkFormat getImageFormat() const { return vkImageFormat_; }
		VkSampleCountFlagBits getSamples() const { return vkSamples_; }
		uint16_t getNumLayers() const { return numLayers_; }
		uint32_t getNumLevels() const { return numLevels_; }
		bool getIsDepthFormat() const { return isDepthFormat_; }
		bool getIsStencilFormat() const { return isStencilFormat_; }
        bool getIsSwapchainImage() const { return isSwapchainImage_; }
		bool getIsOwningVkImage() const { return isOwningVkImage_; }
        inline bool isDepthAttachment() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) > 0; }
        inline bool isStorageImage() const { return (vkUsageFlags_ & VK_IMAGE_USAGE_STORAGE_BIT) > 0; }
        inline bool isAttachment() const { return (vkUsageFlags_ & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) > 0; }
        inline bool isSampledImage() const { return ((vkUsageFlags_ & VK_IMAGE_USAGE_SAMPLED_BIT) > 0); }
    private:
      /*  const VulkanDevice* vulkanDevice = nullptr;
        CommandManager* commandManager = nullptr;
        BufferManager* bufferManager = nullptr;

        VkFormat findDepthFormat();
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        bool hasStencilComponent(VkFormat format);*/
        int w_, h_, comp_;
        const uint8_t* img_ = nullptr;
		VulkanDevice* vulkanDevice_;
        VkImage vkImage_ = VK_NULL_HANDLE;
        VkImageUsageFlags vkUsageFlags_ = 0;
        VkDeviceMemory vkMemory_[1] = { VK_NULL_HANDLE };
        VkFormatProperties vkFormatProperties_ = {};
        VkExtent3D vkExtent_ = { 0, 0, 0 };
        VkImageType vkType_ = VK_IMAGE_TYPE_MAX_ENUM;
        VkFormat vkImageFormat_ = VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits vkSamples_ = VK_SAMPLE_COUNT_1_BIT;
 
        bool isSwapchainImage_ = false;
        bool isOwningVkImage_ = true;
        uint32_t numLevels_ = 1u;
        uint32_t numLayers_ = 1u;
        bool isDepthFormat_ = false;
        bool isStencilFormat_ = false;
        // current image layout
        mutable VkImageLayout vkImageLayout_ =
            VK_IMAGE_LAYOUT_UNDEFINED;
        // precached image views - owned by this VulkanImage
        VkImageView imageView_ = VK_NULL_HANDLE; // all levels
        VkImageView imageViewStorage_ = VK_NULL_HANDLE; // identity swizzle


        bool isDepthOrStencilFormat(Format_e format);
};

void transitionToColorAttachment(VkCommandBuffer buffer, TextureManager* colorTex);

