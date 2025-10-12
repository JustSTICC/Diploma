#pragma once
#include <iostream>
#include <string>
#include <vulkan/vulkan.hpp>
#include <vector>

#ifndef VK_LOG
#define LOG_VK_VERSION_SUPPORT(version)				
#define LOG_VK_EXTENSIONS(extensions)				
#define LOG_VK_LAYERS(layers)						
#define LOG_VK_PHYSICAL_DEVICE(device)				
#define LOG_VK_QUEUE_FAMILIES(queueFamilies)		
#define LOG_VK_SURFACE_CAPABILITIES(capabilities)	
#define LOG_VK_SURFACE_EXTENT(extent)				
#define LOG_VK_SURFACE_EXTENT(extent, prefix)		
#define LOG_VK_SURFACE_FORMAT(format)				
#define LOG_VK_PRESENT_MODES(modes)	
#else
#define LOG_VK_VERSION_SUPPORT(version)				Logger::getInstance()->report_version_support(version);
#define LOG_VK_EXTENSIONS(extensions)				Logger::getInstance()->print_extensions(extensions);
#define LOG_VK_LAYERS(layers)						Logger::getInstance()->print_layers(layers);
#define LOG_VK_PHYSICAL_DEVICE(device)				Logger::getInstance()->log(device);
#define LOG_VK_QUEUE_FAMILIES(queueFamilies)		Logger::getInstance()->log(queueFamilies);
#define LOG_VK_SURFACE_CAPABILITIES(capabilities)	Logger::getInstance()->log(capabilities);
#define LOG_VK_SURFACE_EXTENT(extent)				Logger::getInstance()->log(extent);
#define LOG_VK_SURFACE_EXTENT(extent, prefix)		Logger::getInstance()->log(extent, prefix);
#define LOG_VK_SURFACE_FORMAT(format)				Logger::getInstance()->log(format);
#define LOG_VK_PRESENT_MODES(modes)					Logger::getInstance()->log(modes);


class Logger {
	public:
		static Logger* loggerInstance;
		static Logger* getInstance();

		void report_version_support(uint32_t version);
		void print_extensions(const std::vector<VkExtensionProperties>& extensions);
		void print_layers(const std::vector<VkLayerProperties>& layers);
		void log(const VkPhysicalDevice& device);
		void log(const std::vector<VkQueueFamilyProperties>& queueFamilies);
		void log(const VkSurfaceCapabilitiesKHR& capabilities);
		void log(const VkExtent2D& extent, const char* prefix = "\t");
		void log(const VkSurfaceFormatKHR& format);
		void log(const std::vector<VkPresentModeKHR>& modes);
};
#endif

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData);

