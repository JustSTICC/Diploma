#include "SyncUtils.h"
#include "../validation/VulkanValidator.h"

VkSemaphore createSemaphore(VkDevice device, const char* debugName) {
    const VkSemaphoreCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .flags = 0,
    };
    VkSemaphore semaphore = VK_NULL_HANDLE;
    ASSERT_VK_RESULT(vkCreateSemaphore(device, &ci, nullptr, &semaphore), "Creating semaphore");
    setDebugObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, debugName);
    return semaphore;
}

VkFence createFence(VkDevice device, const char* debugName) {
    const VkFenceCreateInfo ci = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = 0,
    };
    VkFence fence = VK_NULL_HANDLE;
    ASSERT_VK_RESULT(vkCreateFence(device, &ci, nullptr, &fence), "Cerating fence");
    setDebugObjectName(device, VK_OBJECT_TYPE_FENCE, (uint64_t)fence, debugName);
    return fence;
}