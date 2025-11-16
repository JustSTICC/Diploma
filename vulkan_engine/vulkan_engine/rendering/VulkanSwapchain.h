#pragma once

#include "../common/render_def.h"
#include <GLFW/glfw3.h>

#define VK_MAX_SWAPCHAIN_IMAGES 16

class VulkanEngine;
class VulkanDevice;

class VulkanSwapchain{
    public:
        VulkanSwapchain() = default;
        VulkanSwapchain(const VulkanEngine* eng);
        ~VulkanSwapchain();

        void initialize(const VulkanDevice& device, VkSurfaceKHR surface, GLFWwindow* window);
        void cleanup();
        void recreate(GLFWwindow* window);
        Result present(VkSemaphore waitSemaphore);

        void setTimelineWaitValue(uint64_t value, uint32_t index) {
            timelineWaitValues_[index] = value;
		}

        VkSwapchainKHR getSwapChain() const { return swapChain; }
        const std::vector<VkImage>& getImages() const { return swapChainImages; }
        VkFormat getImageFormat() const { return swapChainImageFormat; }
        VkExtent2D getExtent() const { return swapChainExtent; }
        const std::vector<VkImageView>& getImageViews() const { return swapChainImageViews; }
		uint64_t getCurrentFrameIndex() const { return currentFrameIndex_; }
		uint32_t getNumSwapchainImages() const { return numSwapchainImages_; }
        TextureHandle getCurrentTexture();

    private:

        VkQueue graphicsQueue_ = VK_NULL_HANDLE;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        uint32_t numSwapchainImages_ = 0;
        uint32_t currentImageIndex_ = 0; // [0...numSwapchainImages_)
        uint64_t currentFrameIndex_ = 0; // [0...+inf)
        bool getNextImage_ = true;
        VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
        VkSurfaceFormatKHR surfaceFormat_ = { .format = VK_FORMAT_UNDEFINED };
        TextureHandle swapchainTextures_[VK_MAX_SWAPCHAIN_IMAGES] = {};
        VkSemaphore acquireSemaphore_[VK_MAX_SWAPCHAIN_IMAGES] = {};
        VkFence presentFence_[VK_MAX_SWAPCHAIN_IMAGES] = {};
        uint64_t timelineWaitValues_[VK_MAX_SWAPCHAIN_IMAGES] = {};

		const VulkanEngine* eng_;
        const VulkanDevice* vulkanDevice = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;

        VkSwapchainKHR swapChain = VK_NULL_HANDLE;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;

        void createSwapChain(GLFWwindow* window);
        void createImageViews();
        void cleanupSwapChain();

        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
        VkImageUsageFlags chooseImageUsageFlags(const SwapChainSupportDetails details);
};