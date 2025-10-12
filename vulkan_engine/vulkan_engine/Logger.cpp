#include "Logger.h"

#ifdef VK_LOG

Logger* Logger::loggerInstance = nullptr;
Logger* Logger::getInstance() {
	if (loggerInstance == nullptr) {
		loggerInstance = new Logger();
	}
	return loggerInstance;
}

void Logger::report_version_support(uint32_t version) {
	uint32_t major = VK_VERSION_MAJOR(version);
	uint32_t minor = VK_VERSION_MINOR(version);
	uint32_t patch = VK_VERSION_PATCH(version);
	std::cout << "Vulkan API Version Supported: " << major << "." << minor << "." << patch << std::endl;
}

void Logger::print_extensions(const std::vector<VkExtensionProperties>& extensions) {
	std::cout << "Available Vulkan Extensions:" << std::endl;
	for (const auto& ext : extensions) {
		std::cout << "\t" << ext.extensionName << " (spec version " << ext.specVersion << ")" << std::endl;
	}
}

void Logger::print_layers(const std::vector<VkLayerProperties>& layers) {
	std::cout << "Available Vulkan Layers:" << std::endl;
	for (const auto& layer : layers) {
		std::cout << "\t" << layer.layerName << " (spec version " << layer.specVersion << ", implementation version " << layer.implementationVersion << ")" << std::endl;
		std::cout << "\t\t" << layer.description << std::endl;
	}
}

void Logger::log(const VkPhysicalDevice& device) {
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	std::cout << "Physical Device: " << deviceProperties.deviceName << std::endl;
	std::cout << "\tAPI Version: " << VK_VERSION_MAJOR(deviceProperties.apiVersion) << "." << VK_VERSION_MINOR(deviceProperties.apiVersion) << "." << VK_VERSION_PATCH(deviceProperties.apiVersion) << std::endl;
	std::cout << "\tDriver Version: " << deviceProperties.driverVersion << std::endl;
	std::cout << "\tVendor ID: " << deviceProperties.vendorID << std::endl;
	std::cout << "\tDevice ID: " << deviceProperties.deviceID << std::endl;
	std::cout << "\tDevice Type: " << deviceProperties.deviceType << std::endl;
}

void Logger::log(const std::vector<VkQueueFamilyProperties>& queueFamilies) {
	std::cout << "Queue Families:" << std::endl;
	for (size_t i = 0; i < queueFamilies.size(); i++) {
		const auto& qf = queueFamilies[i];
		std::cout << "\tQueue Family " << i << ":" << std::endl;
		std::cout << "\t\tQueue Count: " << qf.queueCount << std::endl;
		std::cout << "\t\tQueue Flags: " << qf.queueFlags << std::endl;
	}
}

void Logger::log(const VkSurfaceCapabilitiesKHR& capabilities) {
	std::cout << "Surface Capabilities:" << std::endl;
	std::cout << "\tMin Image Count: " << capabilities.minImageCount << std::endl;
	std::cout << "\tMax Image Count: " << capabilities.maxImageCount << std::endl;
	std::cout << "\tCurrent Extent: " << capabilities.currentExtent.width << "x" << capabilities.currentExtent.height << std::endl;
	std::cout << "\tMin Image Extent: " << capabilities.minImageExtent.width << "x" << capabilities.minImageExtent.height << std::endl;
	std::cout << "\tMax Image Extent: " << capabilities.maxImageExtent.width << "x" << capabilities.maxImageExtent.height << std::endl;
	std::cout << "\tMax Image Array Layers: " << capabilities.maxImageArrayLayers << std::endl;
	std::cout << "\tSupported Transforms: " << capabilities.supportedTransforms << std::endl;
	std::cout << "\tCurrent Transform: " << capabilities.currentTransform << std::endl;
	std::cout << "\tSupported Composite Alpha: " << capabilities.supportedCompositeAlpha << std::endl;
	std::cout << "\tSupported Usage Flags: " << capabilities.supportedUsageFlags << std::endl;
}

void Logger::log(const VkExtent2D& extent, const char* prefix) {
	std::cout << prefix << "Extent: " << extent.width << "x" << extent.height << std::endl;
}

void Logger::log(const VkSurfaceFormatKHR& format) {
	std::cout << "Surface Format:" << std::endl;
	std::cout << "\tFormat: " << format.format << std::endl;
	std::cout << "\tColor Space: " << format.colorSpace << std::endl;
}

void Logger::log(const std::vector<VkPresentModeKHR>& modes) {
	std::cout << "Present Modes:" << std::endl;
	for (const auto& mode : modes) {
		std::cout << "\t" << mode << std::endl;
	}
}
#endif


VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {
	std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}



