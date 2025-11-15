#include "CommandManager.h"
#include "../core/VulkanDevice.h"
#include <iostream>
#include <stdexcept>
#include <array>
#include "../utils/SyncUtils.h"
#include "../utils/Utils.h"
#include "../validation/VulkanValidator.h"


CommandManager::~CommandManager(){
    cleanup();
}

void CommandManager::initialize(const VulkanDevice& device){
    vulkanDevice = &device;

    createCommandPool();
    createCommandBuffers();

}

void CommandManager::cleanup(){
    waitAll();
    for (CommandBufferWrapper& buf : buffers_) {
        vkDestroyFence(vulkanDevice->getLogicalDevice(), buf.fence_, nullptr);
        vkDestroySemaphore(vulkanDevice->getLogicalDevice(), buf.semaphore_, nullptr);
    }
    vkDestroyCommandPool(vulkanDevice->getLogicalDevice(), commandPool_, nullptr);
}

void CommandManager::createCommandPool(){
    QueueFamilyIndices queueFamilyIndices = vulkanDevice->findQueueFamilies(vulkanDevice->getPhysicalDevice());

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    ASSERT_VK_RESULT(vkCreateCommandPool(vulkanDevice->getLogicalDevice(), &poolInfo, nullptr, &commandPool_), "Creating CommandPool");

}

void CommandManager::createCommandBuffers(){
    const VkCommandBufferAllocateInfo alloacateInfo = {
     .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
     .commandPool = commandPool_,
     .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
     .commandBufferCount = 1
    };
    for (uint32_t i = 0; i != kMaxCommandBuffers; i++) {
        CommandBufferWrapper& buffer = buffers_[i];
        char fenceName[256] = { 0 };
        char semaphoreName[256] = { 0 };
        
        buffer.semaphore_ = createSemaphore(vulkanDevice->getLogicalDevice(), semaphoreName);
        buffer.fence_ = createFence(vulkanDevice->getLogicalDevice(), fenceName);
        vkAllocateCommandBuffers(vulkanDevice->getLogicalDevice(), &alloacateInfo, &buffer.cmdBufAllocated_);
        buffers_[i].handle_.bufferIndex_ = i;
    }
}

const CommandBufferWrapper& CommandManager::acquire() {
    while (!numAvailableCommandBuffers_) {
        purge();
    }

    CommandBufferWrapper* current = nullptr;
    for (CommandBufferWrapper& buf : buffers_) {
        if (buf.cmdBuf_ == VK_NULL_HANDLE) {
            current = &buf;
            break;
        }
    }
    current->handle_.submitId_ = submitCounter_;
    numAvailableCommandBuffers_--;
    current->cmdBuf_ = current->cmdBufAllocated_;
    current->isEncoding_ = true;

    const VkCommandBufferBeginInfo bi = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    ASSERT_VK_RESULT(vkBeginCommandBuffer(current->cmdBuf_, &bi), "Begin CommandBuffer recording ");
    nextSubmitHandle_ = current->handle_;
    return *current;
}

void CommandManager::purge() {
    const uint32_t numBuffers = VK_UTILS_GET_ARRAY_SIZE(buffers_);
    for (uint32_t i = 0; i != numBuffers; i++) {
        const uint32_t index = i + lastSubmitHandle_.bufferIndex_ + 1;
        CommandBufferWrapper& buf = buffers_[index % numBuffers];
        if (buf.cmdBuf_ == VK_NULL_HANDLE || buf.isEncoding_)
            continue;
        const VkResult result = vkWaitForFences(vulkanDevice->getLogicalDevice(), 1, &buf.fence_, VK_TRUE, 0);
        if (result == VK_SUCCESS) {
            vkResetCommandBuffer(
                buf.cmdBuf_, VkCommandBufferResetFlags{ 0 });
            vkResetFences(vulkanDevice->getLogicalDevice(), 1, &buf.fence_);
            buf.cmdBuf_ = VK_NULL_HANDLE;
            numAvailableCommandBuffers_++;
        }
        else {
            if (result != VK_TIMEOUT) {
               ASSERT_VK_RESULT(result, "Fence timeour");
            }
        }
    }
}

SubmitHandle CommandManager::submit(const CommandBufferWrapper& wrapper) {
    vkEndCommandBuffer(wrapper.cmdBuf_);
    VkSemaphoreSubmitInfo waitSemaphores[] = { {}, {} };
    uint32_t numWaitSemaphores = 0;
    if (waitSemaphore_.semaphore) {
        waitSemaphores[numWaitSemaphores++] = waitSemaphore_;
    }
    if (lastSubmitSemaphore_.semaphore) {
        waitSemaphores[numWaitSemaphores++] = lastSubmitSemaphore_;
    }
    VkSemaphoreSubmitInfo signalSemaphores[] = {
    VkSemaphoreSubmitInfo{
     .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
     .semaphore = wrapper.semaphore_,
     .stageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT},
     {},
    };
    uint32_t numSignalSemaphores = 1;
    if (signalSemaphore_.semaphore) {
        signalSemaphores[numSignalSemaphores++] = signalSemaphore_;
    }
    const VkCommandBufferSubmitInfo submitInfo = {
   .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
   .commandBuffer = wrapper.cmdBuf_,
    };
    const VkSubmitInfo2 si = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
      .waitSemaphoreInfoCount = numWaitSemaphores,
      .pWaitSemaphoreInfos = waitSemaphores,
      .commandBufferInfoCount = 1u,
      .pCommandBufferInfos = &submitInfo,
      .signalSemaphoreInfoCount = numSignalSemaphores,
      .pSignalSemaphoreInfos = signalSemaphores,
    };
    vkQueueSubmit2(vulkanDevice->getGraphicsQueue(), 1u, &si, wrapper.fence_);
    lastSubmitSemaphore_.semaphore = wrapper.semaphore_;
    lastSubmitHandle_ = wrapper.handle_;

    waitSemaphore_.semaphore = VK_NULL_HANDLE;
    signalSemaphore_.semaphore = VK_NULL_HANDLE;
    const_cast<CommandBufferWrapper&>(wrapper).isEncoding_ = false;
    submitCounter_++;
    if (!submitCounter_) {
        submitCounter_++;
    }
    return lastSubmitHandle_;
}

VkCommandBuffer CommandManager::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkanDevice->getLogicalDevice(), &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

bool CommandManager::isReady(const SubmitHandle handle) const {
    if (handle.empty()) {
        return true;
    }
    const CommandBufferWrapper& buffers = buffers_[handle.bufferIndex_];
    if (buffers.cmdBuf_ == VK_NULL_HANDLE) {
        return true;
    }
    if (buffers.handle_.submitId_ != handle.submitId_) {
        return true;
    }
    return vkWaitForFences(vulkanDevice->getLogicalDevice(), 1, &buffers.fence_, VK_TRUE, 0) == VK_SUCCESS;
}

void CommandManager::wait(const SubmitHandle handle) {
    if (handle.empty()) {
        vkDeviceWaitIdle(vulkanDevice->getLogicalDevice());
        return;
    }
    if (isReady(handle)) return;
    if (buffers_[handle.bufferIndex_].isEncoding_) {
        return;
    }
    ASSERT_VK_RESULT(vkWaitForFences(vulkanDevice->getLogicalDevice(), 1, &buffers_[handle.bufferIndex_].fence_, VK_TRUE, UINT64_MAX), "Fence timeout");
    purge();
}

void CommandManager::waitAll() {
    VkFence fences[kMaxCommandBuffers];
    uint32_t numFences = 0;
    for (const CommandBufferWrapper& buf : buffers_) {
        if (buf.cmdBuf_ != VK_NULL_HANDLE && !buf.isEncoding_) {
            fences[numFences++] = buf.fence_;
        }
    }
    if (numFences) {
        ASSERT_VK_RESULT(vkWaitForFences(vulkanDevice->getLogicalDevice(), numFences, fences, VK_TRUE, UINT64_MAX),"Fence timeout");
    }
    purge();
}

void CommandManager::signalSemaphore(VkSemaphore semaphore, uint64_t signalValue) {
    VK_ASSERT(signalSemaphore_.semaphore == VK_NULL_HANDLE);

    signalSemaphore_.semaphore = semaphore;
    signalSemaphore_.value = signalValue;
}

void CommandManager::waitSemaphore(VkSemaphore semaphore) {
    if (waitSemaphore_.semaphore == VK_NULL_HANDLE) {
        waitSemaphore_.semaphore = semaphore;
    }
    else {
        std::cout << "Waiting for semaphore" << std::endl;
    }
}

VkSemaphore CommandManager::acquireLastSubmitSemaphore() {
    return std::exchange(lastSubmitSemaphore_.semaphore, VK_NULL_HANDLE);
}

SubmitHandle CommandManager::getLastSubmitHandle() const {
    return lastSubmitHandle_;
}

SubmitHandle CommandManager::getNextSubmitHandle() const {
    return nextSubmitHandle_;
}

void CommandManager::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(vulkanDevice->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkanDevice->getGraphicsQueue());

    vkFreeCommandBuffers(vulkanDevice->getLogicalDevice(), commandPool_, 1, &commandBuffer);
}

void CommandManager::resetCommandBuffer(uint32_t frameIndex) {
    vkResetCommandBuffer(buffers_[frameIndex].cmdBufAllocated_, 0);
}

void CommandManager::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, 
                           VkRenderPass renderPass, VkFramebuffer framebuffer,
                           VkExtent2D extent, VkPipeline graphicsPipeline,
                           VkPipelineLayout pipelineLayout, VkBuffer vertexBuffer,
                           VkBuffer indexBuffer, const std::vector<VkDescriptorSet>& descriptorSets,
                           uint32_t currentFrame, uint32_t indexCount) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult beginCommandBufferResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if ( beginCommandBufferResult != VK_SUCCESS) {
        std::cout << "failed to begin recording command buffer - " << beginCommandBufferResult << std::endl;
        throw std::runtime_error("failed to begin recording command buffer!");
    }else{
        //std::cout << "Successfully started recording command buffer - " << beginCommandBufferResult << std::endl;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indexCount), 1, 0, 0, 0);
}
