#pragma once

#include "../common/render_def.h"
#include <optional>
#include <set>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete(){
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};


class VulkanInstance;

class VulkanDevice {
    public:
        VulkanDevice();
        ~VulkanDevice();

        void initialize(const VulkanInstance& instance, VkSurfaceKHR surface);
        void cleanup();

        VkPhysicalDevice getPhysicalDevice() const { return physicalDevice_; }
        VkDevice getLogicalDevice() const { return device_; }
        VkQueue getGraphicsQueue() const { return graphicsQueue_; }
        VkQueue getPresentQueue() const { return presentQueue_; }
		std::vector<VkFormat> getDeviceDepthFormats() const { return deviceDepthFormats_; }
        VkPhysicalDeviceProperties getPhysicalDeviceProperties() const{
            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(physicalDevice_, &properties);
            return properties;
		}
        uint32_t getFramebufferMSAABitMask() const;
        VkFormat getClosestDepthStencilFormat(Format_e desiredFormat);

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) const;
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const;
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    private:
        const VulkanInstance* vulkanInstance_ = nullptr;
        VkSurfaceKHR surface_ = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
        VkDevice device_ = VK_NULL_HANDLE;
        VkQueue graphicsQueue_ = VK_NULL_HANDLE;
        VkQueue presentQueue_ = VK_NULL_HANDLE;
        std::vector<VkFormat> deviceDepthFormats_;
        VulkanValidator validator;

        VkPhysicalDeviceVulkan13Features vkFeatures13_ = {
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES 
        };
        VkPhysicalDeviceVulkan12Features vkFeatures12_ = {
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
          .pNext = &vkFeatures13_ 
        };
        VkPhysicalDeviceVulkan11Features vkFeatures11_ = {
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
          .pNext = &vkFeatures12_ 
        };
        VkPhysicalDeviceFeatures2 vkFeatures10_ = {
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
          .pNext = &vkFeatures11_ 
        };

        // The pNext chain for properties requires a specific declaration order.
        // The last struct in the chain must be declared first.
        VkPhysicalDeviceDriverProperties vkPhysicalDeviceDriverProperties_ = {
          .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES,
          .pNext = nullptr
        };
        // provided by Vulkan 1.3
        VkPhysicalDeviceVulkan13Properties vkPhysicalDeviceVulkan13Properties_ = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES,
            .pNext = &vkPhysicalDeviceDriverProperties_,
        };
        // provided by Vulkan 1.2
        VkPhysicalDeviceVulkan12Properties vkPhysicalDeviceVulkan12Properties_ = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES,
            .pNext = &vkPhysicalDeviceVulkan13Properties_,
        };
        VkPhysicalDeviceVulkan11Properties vkPhysicalDeviceVulkan11Properties_ = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES,
            .pNext = &vkPhysicalDeviceVulkan12Properties_,
        };
        VkPhysicalDeviceProperties2 vkPhysicalDeviceProperties2_ = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
            .pNext = &vkPhysicalDeviceVulkan11Properties_,
            .properties = {}
        };


        const std::vector<const char*> deviceExtensions_ = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
            VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };

        void pickPhysicalDevice();
        void createLogicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        void getDeviceExtensionProps(VkPhysicalDevice dev, std::vector<VkExtensionProperties>& props, const char* validationLayer = nullptr);

#ifdef VK_LOG
		void logPhysicalDeviceProperties();
#endif
};

