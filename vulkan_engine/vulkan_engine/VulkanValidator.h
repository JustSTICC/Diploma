#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <iostream>
class VulkanValidator
{
public:
	VulkanValidator();
    ~VulkanValidator(){}

    void cleanup(VkInstance instance);

	bool isValidationEnabled() const { return enableValidationLayers; }
	const std::vector<const char*>& getValidationLayers() const { return validationLayers; }
    void setupDebugMessenger(VkInstance instance);
private:
    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    bool enableValidationLayers;

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    bool checkValidationLayerSupport();
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);

