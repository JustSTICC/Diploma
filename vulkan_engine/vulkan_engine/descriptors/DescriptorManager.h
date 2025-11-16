#pragma once 

#include "../common/render_def.h"

struct UniformBufferObject;

class VulkanEngine;
class CommandManager;
struct SubmitHandle;
class DescriptorManager {
    public:
        DescriptorManager(VulkanEngine& eng);
        ~DescriptorManager();

        void initialize(uint32_t maxTextures = 16, uint32_t maxSamplers = 16);
        void cleanup();

  //      void createDescriptorPool(uint32_t maxFramesInFlight);
  //      void createDescriptorSets(VkDescriptorSetLayout descriptorSetLayout, 
  //                           uint32_t maxFramesInFlight,
  //                           const std::vector<VkBuffer>& uniformBuffers,
  //                           VkImageView textureImageView,
  //                           VkSampler textureSampler,
  //                           std::vector<VkDescriptorSet>& descriptorSets);

  //      VkDescriptorPool getDescriptorPool() const { return descriptorPool_; }
		//VkDescriptorSet getDescriptorSet() const { return descriptorSet_; }

  //      void destroyDescriptorPool();

        Result growDescriptorPool(uint32_t maxTextures, uint32_t maxSamplers);
        void updateDescriptorSets(CommandManager* commandManager);
        static VkDescriptorSetLayoutBinding getDSLBinding(uint32_t binding,
            VkDescriptorType descriptorType,
            uint32_t descriptorCount,
            VkShaderStageFlags stageFlags,
            const VkSampler* immutableSamplers);
		VkDescriptorSet getDescriptorSet() const { return descriptorSet_; }
        VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout_; }
        
    private:
        /*const VulkanDevice* vulkanDevice = nullptr;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
       
        uint32_t maxTextures = 0;
        uint32_t maxSamplers = 0;
        uint32_t maxAccelStructs = 0;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;*/
		VulkanEngine& eng_;
        uint32_t currentMaxTextures_ = 16;
        uint32_t currentMaxSamplers_ = 16;
        uint32_t currentMaxAccelStructs_ = 4;
        VkDescriptorSetLayout descriptorSetLayout_ = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
        SubmitHandle lastSubmitHandle = SubmitHandle();
        
};