#include "BufferManager.h"
#include "../core/VulkanEngine.h"
#include "../core/VulkanDevice.h"
#include "../utils/Utils.h"
#include "../utils/ScopeExit.h"

class VulkanEngine;
class VulkanDevice;

//struct UniformBufferObject{
//    glm::mat4 model;
//    glm::mat4 view;
//    glm::mat4 proj;
//};

BufferManager::BufferManager() {}

BufferManager::~BufferManager(){
    //cleanup();
}

//void BufferManager::initialize(const VulkanDevice& device, CommandManager& cmdManager){
//    vulkanDevice = &device;
//    commandManager = &cmdManager;
//}



//void BufferManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory){
//    VkBufferCreateInfo bufferInfo{};
//    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//    bufferInfo.size = size;
//    bufferInfo.usage = usage;
//    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//    if (vkCreateBuffer(vulkanDevice->getLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
//        throw std::runtime_error("failed to create buffer!");
//    }
//
//    VkMemoryRequirements memRequirements;
//    vkGetBufferMemoryRequirements(vulkanDevice->getLogicalDevice(), buffer, &memRequirements);
//
//    VkMemoryAllocateInfo allocInfo{};
//    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//    allocInfo.allocationSize = memRequirements.size;
//    allocInfo.memoryTypeIndex = vulkanDevice->findMemoryType(memRequirements.memoryTypeBits, properties);
//
//    if (vkAllocateMemory(vulkanDevice->getLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
//        throw std::runtime_error("failed to allocate buffer memory!");
//    }
//
//    vkBindBufferMemory(vulkanDevice->getLogicalDevice(), buffer, bufferMemory, 0);
//}

void BufferManager::createBuffer(const BufferDesc& requestedDesc,
    VkPhysicalDevice phyDevice,
    VkDevice device,
    Result* outResult,
    bool useStaging) {
    BufferDesc desc = requestedDesc;
    if (!useStaging && (desc.storage == StorageType_Device)) {
        desc.storage = StorageType_HostVisible;
    }

    // Map engine usage to VkBufferUsageFlags
    vkUsageFlags_ = 0;
    if (desc.usage & BufferUsageBits_Index)   vkUsageFlags_ |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (desc.usage & BufferUsageBits_Vertex)  vkUsageFlags_ |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (desc.usage & BufferUsageBits_Uniform) vkUsageFlags_ |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (desc.usage & BufferUsageBits_Storage) vkUsageFlags_ |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    if (desc.usage & BufferUsageBits_Indirect)vkUsageFlags_ |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    const VkMemoryPropertyFlags memFlags = storageTypeToVkMemoryPropertyFlags(desc.storage);
    vkMemFlags_ = memFlags;
    bufferSize_ = desc.size;

    // Ensure copy usage flags are set correctly for staging/device-local paths
    if (useStaging) {
        if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            vkUsageFlags_ |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // staging buffer
        } else {
            vkUsageFlags_ |= VK_BUFFER_USAGE_TRANSFER_DST_BIT; // device-local upload target
        }
    }

    const VkBufferCreateInfo bufferInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = 0,
        .size = bufferSize_,
        .usage = vkUsageFlags_,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr
    };

    VkResult res = vkCreateBuffer(device, &bufferInfo, nullptr, &vkBuffer_);
    ASSERT_VK_RESULT(res, "vkCreateBuffer failed");

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, vkBuffer_, &requirements);

    // Correctly derive coherence from requested memory properties, not memoryTypeBits
    isCoherentMemory_ = (memFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;

    VkMemoryRequirements2 requirements2{
        .sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2,
        .pNext = nullptr,
        .memoryRequirements = requirements
    };
    allocateMemory(phyDevice, device, &requirements2, memFlags, &vkMemory_);
    vkBindBufferMemory(device, vkBuffer_, vkMemory_, 0);

    if (memFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        res = vkMapMemory(device, vkMemory_, 0, bufferSize_, 0, &mappedPtr_);
        ASSERT_VK_RESULT(res, "vkMapMemory failed");
    }

    Result::setResult(outResult, Result());
}

void BufferManager::flushMappedMemory(
    const VulkanEngine& eng,
    VkDeviceSize offset,
    VkDeviceSize size) const
{
    if (!VK_VERIFY(isMapped())) {
        return;
    }

    const VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = vkMemory_,
        .offset = offset,
        .size = size,
    };
    vkFlushMappedMemoryRanges(eng.vulkanDevice_.get()->getLogicalDevice(), 1, &range);
}

void BufferManager::getBufferSubData(
    const VulkanEngine& eng,
    size_t offset,
    size_t size,
    void* data)
{
    if (!mappedPtr_) return;
    if (!isCoherentMemory_) {
        invalidateMappedMemory(eng, offset, size);
    }
    memcpy(data, (const uint8_t*)mappedPtr_ + offset, size);
}

void BufferManager::bufferSubData(const VulkanEngine& eng, size_t offset, size_t size, const void* data) {
    // only host-visible buffers can be uploaded this way    VK_ASSERT(mappedPtr_);

    if (!mappedPtr_) {
        return;
    }

    VK_ASSERT(offset + size <= bufferSize_);

    if (data) {
        memcpy((uint8_t*)mappedPtr_ + offset, data, size);
    }
    else {
        memset((uint8_t*)mappedPtr_ + offset, 0, size);
    }

    if (!isCoherentMemory_) {
        flushMappedMemory(eng, offset, size);
    }
}

void BufferManager::invalidateMappedMemory(const VulkanEngine& eng, VkDeviceSize offset, VkDeviceSize size) const {
    if (!VK_VERIFY(isMapped())) {
        return;
    }

    const VkMappedMemoryRange range = {
        .sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .memory = vkMemory_,
        .offset = offset,
        .size = size,
    };
    vkInvalidateMappedMemoryRanges(eng.vulkanDevice_.get()->getLogicalDevice(), 1, &range);
    
}


//void BufferManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size){
//    VkCommandBuffer commandBuffer = commandManager->beginSingleTimeCommands();
//
//    VkBufferCopy copyRegion{};
//    copyRegion.size = size;
//    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
//
//    commandManager->endSingleTimeCommands(commandBuffer);
//}
//
//
//void BufferManager::createUniformBuffer(uint32_t maxFramesInFlight, std::vector<VkBuffer>& uniformBuffers, 
//                             std::vector<VkDeviceMemory>& uniformBuffersMemory, 
//                             std::vector<void*>& uniformBuffersMapped){
//    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
//
//    uniformBuffers.resize(maxFramesInFlight);
//    uniformBuffersMemory.resize(maxFramesInFlight);
//    uniformBuffersMapped.resize(maxFramesInFlight);
//
//    for (size_t i = 0; i < maxFramesInFlight; i++) {
//        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
//        vkMapMemory(vulkanDevice->getLogicalDevice(), uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
//    }
//}
//
//void BufferManager::createIndexBuffer(const std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory){
//    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
//
//    VkBuffer stagingBuffer;
//    VkDeviceMemory stagingBufferMemory;
//    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
//    void* data;
//    vkMapMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
//    memcpy(data, indices.data(), (size_t) bufferSize);
//    vkUnmapMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory);
//
//    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
//    copyBuffer(stagingBuffer, indexBuffer, bufferSize);
//
//    vkDestroyBuffer(vulkanDevice->getLogicalDevice(), stagingBuffer, nullptr);
//    vkFreeMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory, nullptr);
//}
//
//void BufferManager::createVertexBuffer(const std::vector<StandardVertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory){
//    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
//
//    VkBuffer stagingBuffer;
//    VkDeviceMemory stagingBufferMemory;
//    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
//    void* data;
//    vkMapMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
//    memcpy(data, vertices.data(), (size_t) bufferSize);
//    vkUnmapMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory);
//
//    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
//    copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
//
//    vkDestroyBuffer(vulkanDevice->getLogicalDevice(), stagingBuffer, nullptr);
//    vkFreeMemory(vulkanDevice->getLogicalDevice(), stagingBufferMemory, nullptr);
//}


//void BufferManager::destroyBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
//    if (buffer != VK_NULL_HANDLE) {
//        vkDestroyBuffer(vulkanDevice->getLogicalDevice(), buffer, nullptr);
//        buffer = VK_NULL_HANDLE;
//    }
//    if (bufferMemory != VK_NULL_HANDLE) {
//        vkFreeMemory(vulkanDevice->getLogicalDevice(), bufferMemory, nullptr);
//        bufferMemory = VK_NULL_HANDLE;
//    }
//}