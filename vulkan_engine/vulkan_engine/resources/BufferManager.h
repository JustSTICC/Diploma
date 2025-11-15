#pragma once
#include "../common/render_def.h"
//#include "../rendering/CommandManager.h"
//#include "../common/Vertex.h"
//#include "../common/VertexTypes.h"

struct UniformBufferObject;
class VulkanEngine;

class BufferManager {
    public:
        BufferManager();
        ~BufferManager();

        VkBuffer vkBuffer_ = VK_NULL_HANDLE;

        //void initialize(const VulkanDevice& device, CommandManager& commandManager);
        void destroy(BufferHandle handle);

  /*      void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, 
                     VkBuffer& buffer, VkDeviceMemory& bufferMemory);*/
        void createBuffer(const BufferDesc& requestedDesc,
            VkPhysicalDevice phyDevice,
            VkDevice device,
            Result* outResult,
            bool useStaging);

		VkDeviceSize getBufferSize() const { return bufferSize_; }
		VkDeviceAddress getBufferDeviceAddress() const { return vkDeviceAddress_; }

        //void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        //void createVertexBuffer(const std::vector<StandardVertex>& vertices, VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory);
        //void createIndexBuffer(const std::vector<uint32_t>& indices, VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory);
        //void createUniformBuffer(uint32_t maxFramesInFlight, std::vector<VkBuffer>& uniformBuffers, 
        //                     std::vector<VkDeviceMemory>& uniformBuffersMemory, 
        //                     std::vector<void*>& uniformBuffersMapped);

        //void destroyBuffer(VkBuffer& buffer, VkDeviceMemory& bufferMemory);

        uint8_t* getMappedPtr() const { return (uint8_t*)mappedPtr_; }
        bool isMapped() const { return mappedPtr_ != nullptr; }
        void bufferSubData(const VulkanEngine& eng, size_t offset, size_t size, const void* data);
        void getBufferSubData(const VulkanEngine& eng,size_t offset, size_t size, void* data);
        void flushMappedMemory(const VulkanEngine& eng, VkDeviceSize offset, VkDeviceSize size) const;
        void invalidateMappedMemory(const VulkanEngine& eng, VkDeviceSize offset, VkDeviceSize size) const;

		VkBufferUsageFlags getUsageFlags() const { return vkUsageFlags_; }
		VkDeviceMemory getVkMemory() const { return vkMemory_; }
        void* mappedPtr_ = nullptr;

    private:
  /*      const VulkanDevice* vulkanDevice = nullptr;
        CommandManager* commandManager = nullptr;*/

        VkDeviceMemory vkMemory_ = VK_NULL_HANDLE;
        VkDeviceAddress vkDeviceAddress_ = 0;
        VkDeviceSize bufferSize_ = 0;
        VkBufferUsageFlags vkUsageFlags_ = 0;
        VkMemoryPropertyFlags vkMemFlags_ = 0;
 
        bool isCoherentMemory_ = false;

};