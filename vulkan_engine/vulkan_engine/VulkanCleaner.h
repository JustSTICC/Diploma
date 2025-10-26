#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <deque>
#include <functional>

#define VK_PUSH_INSTANCE_DELETER(func) VulkanCleaner::pushInstanceDeleter(func)
#define VK_PUSH_DEVICE_DELETER(func) VulkanCleaner::pushDeviceDeleter(func)
#define VK_FLUSH_INSTANCE_DELETERS(instance) VulkanCleaner::getInstance()->flushInstanceDeleters(instance)
#define VK_FLUSH_DEVICE_DELETERS(device) VulkanCleaner::getInstance()->flushDeviceDeleters(device)

class VulkanCleaner
{
public:
	static VulkanCleaner* cleanerInstance;
	static VulkanCleaner* getInstance();
	
	static void pushInstanceDeleter(std::function<void(VkInstance)> func);
	static void pushDeviceDeleter(std::function<void(VkDevice)> func);
	void flushInstanceDeleters(VkInstance instance);
	void flushDeviceDeleters(VkDevice device);
	
private:
		std::deque<std::function<void(VkInstance)>> instanceDeletionQueue;
		std::deque<std::function<void(VkDevice)>> deviceDeletionQueue;


};

