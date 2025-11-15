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

        const std::vector<const char*> deviceExtensions_ = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
            VK_EXT_SHADER_OBJECT_EXTENSION_NAME,
            VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
        };

        void pickPhysicalDevice();
        void createLogicalDevice();
        bool isDeviceSuitable(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);

#ifdef VK_LOG
		void logPhysicalDeviceProperties();
#endif
};

