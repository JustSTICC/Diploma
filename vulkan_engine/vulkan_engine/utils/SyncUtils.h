#pragma once
#include <vulkan/vulkan.h>

VkSemaphore createSemaphore(VkDevice device, const char* debugName);
VkFence createFence(VkDevice device, const char* debugName);
