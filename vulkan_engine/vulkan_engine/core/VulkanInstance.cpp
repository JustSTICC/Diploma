#include "VulkanInstance.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include "../Logger.h"
#include "../utils/Utils.h"

#ifndef VK_DEBUG
    const bool ENABLE_VALIDATION_LAYERS = false;
#else
    const bool ENABLE_VALIDATION_LAYERS = true;
#endif


VulkanInstance::VulkanInstance(){
}

VulkanInstance::~VulkanInstance(){
    cleanup();
}

void VulkanInstance::initialize(){
    createInstance();
    validator.setupDebugMessenger(&instance);
}

void VulkanInstance::cleanup(){
	validator.cleanup(&instance);

    if(instance != VK_NULL_HANDLE){
        vkDestroyInstance(instance, nullptr);
        instance = VK_NULL_HANDLE;
    }
}

void VulkanInstance::createInstance(){
    if (validator.isValidationEnabled() && !validator.checkValidationLayerSupport()) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    uint32_t version;
	vkEnumerateInstanceVersion(&version);
	LOG_VK_VERSION_SUPPORT(version);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan Diploma Project";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "G.I. Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    auto extensions = getRequiredExtensions();

    VkValidationFeatureEnableEXT validationFeaturesEnabled[] = {
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
    };
    const VkValidationFeaturesEXT features = {
      .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
      .enabledValidationFeatureCount = validator.isValidationEnabled() ?
        (uint32_t)VK_UTILS_GET_ARRAY_SIZE(validationFeaturesEnabled) : 0u,
      .pEnabledValidationFeatures = isValidationEnabled() ?
        validationFeaturesEnabled : nullptr,
    };
    VkBool32 gpuav_descriptor_checks = VK_FALSE;
    VkBool32 gpuav_indirect_draws_buffers = VK_FALSE;
    VkBool32 gpuav_post_process_descriptor_indexing = VK_FALSE;
#define LAYER_SETTINGS_BOOL32(name, var)              \
    VkLayerSettingEXT {                                 \
        .pLayerName = validator.getValidationLayers()[0],        \
        .pSettingName = name,                             \
        .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,         \
        .valueCount = 1,                                  \
        .pValues = var }
    const VkLayerSettingEXT settings[] = {
        LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks",
        &gpuav_descriptor_checks),
        LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers",
        &gpuav_indirect_draws_buffers),
        LAYER_SETTINGS_BOOL32(
        "gpuav_post_process_descriptor_indexing",
        &gpuav_post_process_descriptor_indexing),
    };
#undef LAYER_SETTINGS_BOOL32
    const VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
        .pNext = validator.isValidationEnabled() ? &features : nullptr,
        .settingCount = (uint32_t)VK_UTILS_GET_ARRAY_SIZE(settings),
        .pSettings = settings
    };

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pNext = &layerSettingsCreateInfo;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (validator.isValidationEnabled()) {
        validator.setUtilMessenger(&createInfo);
    }
    else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if(result != VK_SUCCESS){
        std::cerr << "Failed to create vulkan instance - " << result << std::endl;
        throw std::runtime_error("failed to create an instance!");
    }else {
        std::cout << "Vulkan instance created successfully - " << result << std::endl;
    }
    //CreateDebugUtilsMessengerEXT(&instance);

}

//void VulkanInstance::setupDebugMessenger(){
//    if (!enableValidationLayers) return;
//
//    VkDebugUtilsMessengerCreateInfoEXT createInfo;
//    populateDebugMessengerCreateInfo(createInfo);
//
//    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
//        throw std::runtime_error("failed to set up debug messenger!");
//    }
//}
//
//bool VulkanInstance::checkValidationLayerSupport(){
//    uint32_t layerCount;
//    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
//    std::vector<VkLayerProperties> availableLayers(layerCount);
//    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
//    
//    for (const char* layerName : validationLayers) {
//        bool layerFound = false;
//
//        for (const auto& layerProperties : availableLayers) {
//            if (strcmp(layerName, layerProperties.layerName) == 0) {
//                layerFound = true;
//                break;
//            }
//        }
//
//        if (!layerFound) {
//            return false;
//        }
//    }
//    return true;
//}

std::vector<const char*> VulkanInstance::getRequiredExtensions(){
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    if (validator.isValidationEnabled()) {
        extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    }

    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    return extensions;
}

//void VulkanInstance::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo){
//    createInfo = {};
//    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
//    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
//    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
//    createInfo.pfnUserCallback = debugCallback;
//}
//
//VKAPI_ATTR VkBool32 VKAPI_CALL VulkanInstance::debugCallback(
//    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
//    VkDebugUtilsMessageTypeFlagsEXT messageType,
//    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
//    void* pUserData)
//{
//    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
//    return VK_FALSE;
//}

//VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, 
//    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
//    const VkAllocationCallbacks* pAllocator, 
//    VkDebugUtilsMessengerEXT* pDebugMessenger)
//{
//    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
//    if (func != nullptr) {
//        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
//    } else {
//        return VK_ERROR_EXTENSION_NOT_PRESENT;
//    }
//}
//
//void DestroyDebugUtilsMessengerEXT(VkInstance instance, 
//    VkDebugUtilsMessengerEXT debugMessenger, 
//    const VkAllocationCallbacks* pAllocator)
//{
//    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
//    if (func != nullptr) {
//        func(instance, debugMessenger, pAllocator);
//    }
//}