#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include <iostream>
#include <stdexcept>
#include <set>
#include "../utils/Utils.h"
#include "../Logger.h"

VulkanDevice::VulkanDevice(){
}

VulkanDevice::~VulkanDevice(){
    cleanup();
}

void VulkanDevice::initialize(const VulkanInstance& instance, VkSurfaceKHR surface){
    this->vulkanInstance_ = &instance;
    this->surface_ = surface;

    pickPhysicalDevice();
    createLogicalDevice();
}

void VulkanDevice::cleanup(){
    if (device_ != VK_NULL_HANDLE){
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
}

void VulkanDevice::pickPhysicalDevice(){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(vulkanInstance_->getInstance(), &deviceCount, nullptr);

    if(deviceCount == 0){
        std::cerr << "failed to find physical device" << std::endl;
        throw std::runtime_error("failed to find physical device");
    }else{
        std::cout << "no. of GPUs - " << deviceCount << std::endl;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(vulkanInstance_->getInstance(), &deviceCount, devices.data());
    for(const auto& device: devices){
        if(isDeviceSuitable(device)){
            LOG_VK_PHYSICAL_DEVICE(device);
            physicalDevice_ = device;
            break;
        }
    }

    if(physicalDevice_ == VK_NULL_HANDLE){
        std::cerr << "failed to find suitable GPU" << std::endl;
        throw std::runtime_error("failed to find suitable GPU");
    }else{
        std::cout << "Suitable gpu found - " << physicalDevice_ << std::endl;
#ifdef VK_LOG
        logPhysicalDeviceProperties();
#endif // VK_LOG

    }
}

void VulkanDevice::createLogicalDevice(){
    std::vector<VkExtensionProperties> allDeviceExtensions;
    getDeviceExtensionProps(
        physicalDevice_, allDeviceExtensions);
    if (validator.isValidationEnabled()) {
        for (const char* layer : validator.getValidationLayers())
            getDeviceExtensionProps(
                physicalDevice_, allDeviceExtensions, layer);
    }
    vkGetPhysicalDeviceFeatures2(physicalDevice_, &vkFeatures10_);
    vkGetPhysicalDeviceProperties2(physicalDevice_, &vkPhysicalDeviceProperties2_);
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice_);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
	VkPhysicalDeviceShaderObjectFeaturesEXT shaderObjectFeatures{};
	shaderObjectFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_OBJECT_FEATURES_EXT;
	VkPhysicalDeviceVulkan13Features vulkan13Features{};
	vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
	vulkan13Features.synchronization2 = VK_TRUE;
	vulkan13Features.dynamicRendering = VK_TRUE;
	shaderObjectFeatures.pNext = &vulkan13Features;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions_.data();
    if(vulkanInstance_->isValidationEnabled()){
        const auto& validationLayers = vulkanInstance_->getValidationLayers();
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }else{
        createInfo.enabledLayerCount = 0;
    }
	createInfo.pNext = &shaderObjectFeatures;

    VkResult result = vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_);
    if( result != VK_SUCCESS){
        std::cout << "Failed to create logical device - " << result << std::endl;
        throw std::runtime_error("Failed to create logical device");
    }else{
        std::cout << "Successfully created logical device - " << result << std::endl;
    }

    vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
}

bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice device) {
    
    QueueFamilyIndices indices = findQueueFamilies(device);
    bool extensionsSupported = checkDeviceExtensionSupport(device);
    std::cout << "Extensions are supported by the device - " << extensionsSupported << std::endl;
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        std::cout << "Swapchain adequate for the device - " << swapChainAdequate << std::endl;
    }
    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void VulkanDevice::getDeviceExtensionProps(VkPhysicalDevice dev, std::vector<VkExtensionProperties>& props, const char* validationLayer) {
    uint32_t numExtensions = 0;
    vkEnumerateDeviceExtensionProperties(dev, validationLayer, &numExtensions, nullptr);
    std::vector<VkExtensionProperties> p(numExtensions);
    vkEnumerateDeviceExtensionProperties(dev, validationLayer, &numExtensions, p.data());
    props.insert(props.end(), p.begin(), p.end());
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice device){
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
    LOG_VK_EXTENSIONS(availableExtensions);

    std::set<std::string> requiredExtensions(deviceExtensions_.begin(), deviceExtensions_.end());

    for(const auto& extension: availableExtensions){
        requiredExtensions.erase(extension.extensionName);
    }
    
    return requiredExtensions.empty();
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice device) const{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::cout << "Queue Family Count - "<< queueFamilyCount << std::endl;
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    std::cout << "Queue family properties - " << queueFamilies.data()->queueFlags << std::endl;
    LOG_VK_QUEUE_FAMILIES(queueFamilies);
    for(int i=0 ; i<queueFamilies.size(); i++){
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if(queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT){
            indices.graphicsFamily = i;
        }

        if(indices.isComplete()){
            break;
        }
    }

    return indices;
}

SwapChainSupportDetails VulkanDevice::querySwapChainSupport(VkPhysicalDevice device) const{
    /*const VkFormat depthFormats[] = {
    VK_FORMAT_D32_SFLOAT_S8_UINT,
    VK_FORMAT_D24_UNORM_S8_UINT,
    VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT,
    VK_FORMAT_D16_UNORM };
    for (const auto& depthFormat : depthFormats) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(device, depthFormat, &formatProps);
        if (formatProps.optimalTilingFeatures) {
            deviceDepthFormats_.push_back(depthFormat);
        }
    }
    if (surface_ == VK_NULL_HANDLE) {
        return ;
    }*/

    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
    if(formatCount !=0 ){
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
    if(presentModeCount != 0){
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, details.presentModes.data());
    }

    return details;
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);

    for(uint32_t i = 0; i < memProperties.memoryTypeCount; i++){
        if((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties){
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type!");
}

VkFormat VulkanDevice::getClosestDepthStencilFormat(Format_e desiredFormat){
    const std::vector<VkFormat> compatibleDepthStencilFormatList = getCompatibleDepthStencilFormats(desiredFormat);

    // check if any of the format in compatible list is supported
    for (VkFormat format : compatibleDepthStencilFormatList) {
        if (std::find(deviceDepthFormats_.cbegin(), deviceDepthFormats_.cend(), format) != deviceDepthFormats_.cend()) {
            return format;
        }
    }

    // no matching found, choose the first supported format
    return !deviceDepthFormats_.empty() ? deviceDepthFormats_[0] : VK_FORMAT_D24_UNORM_S8_UINT;
    };

uint32_t VulkanDevice::getFramebufferMSAABitMask() const {
    const VkPhysicalDeviceLimits& limits = getPhysicalDeviceProperties().limits;
    return limits.framebufferColorSampleCounts & limits.framebufferDepthSampleCounts;
}

#ifdef VK_LOG
    void VulkanDevice::logPhysicalDeviceProperties() {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice_, &deviceProperties);
        std::cout << "Device Name: " << deviceProperties.deviceName << std::endl;
        std::cout << "API Version: " 
                  << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "."
                  << VK_VERSION_MINOR(deviceProperties.apiVersion) << "."
                  << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
        std::cout << "Driver Version: " 
                  << VK_VERSION_MAJOR(deviceProperties.driverVersion) << "."
                  << VK_VERSION_MINOR(deviceProperties.driverVersion) << "."
                  << VK_VERSION_PATCH(deviceProperties.driverVersion) << std::endl;
        std::cout << "Vendor ID: " << deviceProperties.vendorID << std::endl;
        std::cout << "Device ID: " << deviceProperties.deviceID << std::endl;
        std::cout << "Device Type: " << deviceProperties.deviceType << std::endl;
	}
#endif // VK_LOG


