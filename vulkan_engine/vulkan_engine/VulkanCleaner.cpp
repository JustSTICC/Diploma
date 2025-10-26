#include "VulkanCleaner.h"

VulkanCleaner* VulkanCleaner::cleanerInstance = nullptr;
VulkanCleaner* VulkanCleaner::getInstance() {
	if (cleanerInstance == nullptr) {
		cleanerInstance = new VulkanCleaner();
	}
	return cleanerInstance;
}

void VulkanCleaner::pushInstanceDeleter(std::function<void(VkInstance)> func) {
	getInstance()->instanceDeletionQueue.push_back(func);
}
void VulkanCleaner::pushDeviceDeleter(std::function<void(VkDevice)> func) {
	getInstance()->deviceDeletionQueue.push_back(func);
}

void VulkanCleaner::flushInstanceDeleters(VkInstance instance) {
	while (!instanceDeletionQueue.empty()) {
		instanceDeletionQueue.back()(instance);
		instanceDeletionQueue.pop_back();
	}
}
void VulkanCleaner::flushDeviceDeleters(VkDevice device) {
	while (!deviceDeletionQueue.empty()) {
		deviceDeletionQueue.back()(device);
		deviceDeletionQueue.pop_back();
	}
}