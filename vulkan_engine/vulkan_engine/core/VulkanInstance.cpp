#include "VulkanInstance.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include "../Logger.h"

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
    appInfo.applicationVersion = VK_API_VERSION_1_4;
    appInfo.pEngineName = "G.I. Engine";
    appInfo.engineVersion = VK_API_VERSION_1_4;
    appInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    if (validator.isValidationEnabled()) {
        validator.setUtilMessenger(&createInfo);
    } else {
        createInfo.enabledLayerCount = 0;
        createInfo.pNext = nullptr;
    }

    auto extensions = getRequiredExtensions();
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if(result != VK_SUCCESS){
        std::cerr << "Failed to create vulkan instance - " << result << std::endl;
        throw std::runtime_error("failed to create an instance!");
    }else {
        std::cout << "Vulkan instance created successfully - " << result << std::endl;
        VK_PUSH_INSTANCE_DELETER([this](VkInstance instance) {
                vkDestroyInstance(instance, nullptr);
            });
    }

}



std::vector<const char*> VulkanInstance::getRequiredExtensions(){
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    if (validator.isValidationEnabled()) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

    return extensions;
}

