#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <string>
#include "../validation/VulkanValidator.h"

class VulkanInstance{
    public:
        VulkanInstance();
        ~VulkanInstance();

        VulkanInstance(const VulkanInstance&) = delete;
        VulkanInstance& operator=(const VulkanInstance&) = delete;
        
        void initialize();

        void cleanup();

        VkInstance getInstance() const { return instance; }

        const std::vector<const char*>& getValidationLayers() const { return validator.getValidationLayers(); }

        bool isValidationEnabled() const { return validator.isValidationEnabled(); }

    private:
        VkInstance instance = VK_NULL_HANDLE;
		VulkanValidator validator;

        void createInstance();
        std::vector<const char*> getRequiredExtensions();
};

