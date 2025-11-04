#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include <iostream>


#define ASSERT_VK_RESULT(result, msg) \
    if (result != VK_SUCCESS) { \
        std::cerr << msg << " - Error Code: " << result << std::endl; \
        throw std::runtime_error(msg); \
    } \

class VulkanValidator
{
public:
	VulkanValidator();
    ~VulkanValidator(){}

    void cleanup(VkInstance* instance);

	bool isValidationEnabled() const { return enableValidationLayers; }

	const std::vector<const char*>& getValidationLayers() const { return validationLayers; }

    void setupDebugMessenger(VkInstance* instance);

    bool checkValidationLayerSupport();

    void setUtilMessenger(VkInstanceCreateInfo* createInfo);
private:

    VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
    bool enableValidationLayers;

	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
};

VkResult CreateDebugUtilsMessengerEXT(VkInstance* instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger);

void DestroyDebugUtilsMessengerEXT(VkInstance* instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks* pAllocator);

