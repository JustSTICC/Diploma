#pragma once
#include "../common/render_def.h"

class VulkanDevice;

class CommandManager {
    public:
       

        CommandManager(){}
        ~CommandManager();

        static constexpr uint32_t kMaxCommandBuffers = 64;
        void initialize(const VulkanDevice& device);
        void cleanup();

        VkCommandPool getCommandPool() const { return commandPool_; }
        VkCommandBuffer getCommandBuffer(uint32_t frameIndex) const { return buffers_[frameIndex].cmdBufAllocated_; }

        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commmandBuffer);

        void resetCommandBuffer(uint32_t frameIndex);
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, 
                           VkRenderPass renderPass, VkFramebuffer framebuffer,
                           VkExtent2D extent, VkPipeline graphicsPipeline,
                           VkPipelineLayout pipelineLayout, VkBuffer vertexBuffer,
                           VkBuffer indexBuffer, const std::vector<VkDescriptorSet>& descriptorSets,
                           uint32_t currentFrame, uint32_t indexCount);

        //NEW
        const CommandBufferWrapper& acquire();
        SubmitHandle submit(const CommandBufferWrapper& wrapper);
        void waitSemaphore(VkSemaphore semaphore);
        void signalSemaphore(VkSemaphore semaphore, uint64_t signalValue);
        VkSemaphore acquireLastSubmitSemaphore();
        SubmitHandle getLastSubmitHandle() const;
        SubmitHandle getNextSubmitHandle() const;
        bool isReady(SubmitHandle handle) const;
        void wait(SubmitHandle handle);
        void waitAll();

    private:
        void purge();
        VkCommandPool commandPool_ = VK_NULL_HANDLE;
        CommandBufferWrapper buffers_[kMaxCommandBuffers];
        SubmitHandle lastSubmitHandle_ = SubmitHandle();
        SubmitHandle nextSubmitHandle_ = SubmitHandle();
        uint32_t numAvailableCommandBuffers_ = kMaxCommandBuffers;
        uint32_t submitCounter_ = 1;

        VkSemaphoreSubmitInfo lastSubmitSemaphore_ = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                              .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSemaphoreSubmitInfo waitSemaphore_ = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                                .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        VkSemaphoreSubmitInfo signalSemaphore_ = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                                                  .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT };
        friend class VulkanDevice;
        const VulkanDevice* vulkanDevice = nullptr;
        //std::vector<VkCommandBuffer> commandBuffers;

        void createCommandPool();
        void createCommandBuffers();
};