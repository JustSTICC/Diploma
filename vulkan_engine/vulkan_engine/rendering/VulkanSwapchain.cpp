#include "VulkanSwapchain.h"
#include "../core/VulkanEngine.h"
#include "../core/VulkanDevice.h"
#include "../rendering/CommandManager.h"
#include "../utils/SyncUtils.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include <limits>

VulkanSwapchain::VulkanSwapchain(const VulkanEngine* eng){
	eng_ = eng;
}

VulkanSwapchain::~VulkanSwapchain(){
    cleanup();
}

void VulkanSwapchain::initialize(const VulkanDevice& device, VkSurfaceKHR surface, GLFWwindow* window){
    this->vulkanDevice = &device;
    this->surface = surface;

    createSwapChain(window);
    createImageViews();
}

void VulkanSwapchain::cleanup(){
    cleanupSwapChain();
}

void VulkanSwapchain::recreate(GLFWwindow* window){
    cleanupSwapChain();
    createSwapChain(window);
    createImageViews();
}

Result VulkanSwapchain::present(VkSemaphore waitSemaphore) {

    const VkSwapchainPresentFenceInfoEXT fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_PRESENT_FENCE_INFO_EXT,
        .swapchainCount = 1,
        .pFences = &presentFence_[currentImageIndex_],
    };
    const VkPresentInfoKHR pi = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext =  nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &waitSemaphore,
        .swapchainCount = 1u,
        .pSwapchains = &swapchain_,
        .pImageIndices = &currentImageIndex_,
    };
    /*if (eng_.has_EXT_swapchain_maintenance1_) {
        if (!presentFence_[currentImageIndex_]) {
            presentFence_[currentImageIndex_] = createFence(vulkanDevice->getLogicalDevice(), "Fence: present-fence");
        }
    }*/
    VkResult r = vkQueuePresentKHR(graphicsQueue_, &pi);
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR && r != VK_ERROR_OUT_OF_DATE_KHR) {
        VK_ASSERT(r);
    }

    // Ready to call acquireNextImage() on the next getCurrentVulkanTexture();
    getNextImage_ = true;
    currentFrameIndex_++;

    return Result();
}


void VulkanSwapchain::createSwapChain(GLFWwindow* window){
    SwapChainSupportDetails swapChainSupport = vulkanDevice->querySwapChainSupport(vulkanDevice->getPhysicalDevice());

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, window);
    VkImageUsageFlags usageFlags= chooseImageUsageFlags(swapChainSupport);


    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = usageFlags;

    QueueFamilyIndices indices = vulkanDevice->findQueueFamilies(vulkanDevice->getPhysicalDevice());
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(vulkanDevice->getLogicalDevice(), &createInfo, nullptr, &swapChain);
	ASSERT_VK_RESULT(result, " Creating swapchain");

    vkGetSwapchainImagesKHR(vulkanDevice->getLogicalDevice(), swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkanDevice->getLogicalDevice(), swapChain, &imageCount, swapChainImages.data());
    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void VulkanSwapchain::createImageViews(){
    swapChainImageViews.resize(swapChainImages.size());
    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapChainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapChainImageFormat;
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(vulkanDevice->getLogicalDevice(), &viewInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image view!");
        }
    }
}

void VulkanSwapchain::cleanupSwapChain() {
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(vulkanDevice->getLogicalDevice(), imageView, nullptr);
    }
    swapChainImageViews.clear();

    if (swapChain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkanDevice->getLogicalDevice(), swapChain, nullptr);
        swapChain = VK_NULL_HANDLE;
    }
}

VkSurfaceFormatKHR VulkanSwapchain::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
#ifdef VK_LOG
			std::cout << "Present mode: MAILBOX" << std::endl;
#endif 
            return availablePresentMode;
        }
    }
#ifdef VK_LOG
    std::cout << "Present mode: FIFO" << std::endl;
#endif 
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actualExtent;
    }
}

VkImageUsageFlags VulkanSwapchain::chooseImageUsageFlags(const SwapChainSupportDetails details) {
    VkImageUsageFlags usageFlags =
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT |
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    const bool isStorageSupported =
        (details.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_STORAGE_BIT) > 0;

    const bool isTilingOptimalSupported =
        (details.props.optimalTilingFeatures & VK_IMAGE_USAGE_STORAGE_BIT) > 0;
    if (isStorageSupported && isTilingOptimalSupported) {
        usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    return usageFlags;
}

TextureHandle VulkanSwapchain::getCurrentTexture() {


    if (getNextImage_) {
        if (presentFence_[currentImageIndex_]) {
            // VK_EXT_swapchain_maintenance1: before acquiring again, wait for the presentation operation to finish
            VK_ASSERT(vkWaitForFences(vulkanDevice->getLogicalDevice(), 1, &presentFence_[currentImageIndex_], VK_TRUE, UINT64_MAX));
            VK_ASSERT(vkResetFences(vulkanDevice->getLogicalDevice(), 1, &presentFence_[currentImageIndex_]));
        }
        const VkSemaphoreWaitInfo waitInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
            .semaphoreCount = 1,
            .pSemaphores = &eng_->timelineSemaphore_,
            .pValues = &timelineWaitValues_[currentImageIndex_],
        };
        VK_ASSERT(vkWaitSemaphores(vulkanDevice->getLogicalDevice(), &waitInfo, UINT64_MAX));
        // when timeout is set to UINT64_MAX, we wait until the next image has been acquired
        VkSemaphore acquireSemaphore = acquireSemaphore_[currentImageIndex_];
        VkResult r = vkAcquireNextImageKHR(vulkanDevice->getLogicalDevice(), swapchain_, UINT64_MAX, acquireSemaphore, VK_NULL_HANDLE, &currentImageIndex_);
        if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR && r != VK_ERROR_OUT_OF_DATE_KHR) {
            VK_ASSERT(r);
        }
        getNextImage_ = false;
        
        eng_->commandManager_->waitSemaphore(acquireSemaphore);
    }

    if (VK_VERIFY(currentImageIndex_ < numSwapchainImages_)) {
        return swapchainTextures_[currentImageIndex_];
    }

    return {};
}