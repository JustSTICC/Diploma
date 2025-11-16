#include "DescriptorManager.h"
#include "../core/VulkanEngine.h"
#include "../core/VulkanDevice.h"
#include "../rendering/CommandManager.h"
#include "../resources/TextureManager.h"
#include <iostream>
#include <stdexcept>
#include <glm/glm.hpp>

struct UniformBufferObject{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

DescriptorManager::DescriptorManager(VulkanEngine& eng) : eng_(eng) {}

DescriptorManager::~DescriptorManager() {
    cleanup();
}

void DescriptorManager::initialize(uint32_t maxTextures, uint32_t maxSamplers) {
    growDescriptorPool(maxTextures, maxSamplers);
}

void DescriptorManager::cleanup() {
    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        eng_.vulkanDevice_.get()->getLogicalDevice();  // cleanup logic if needed
    }
    if (descriptorPool_ != VK_NULL_HANDLE) {
        eng_.vulkanDevice_.get()->getLogicalDevice();  // cleanup logic if needed
    }
}

Result DescriptorManager::growDescriptorPool(uint32_t maxTextures, uint32_t maxSamplers) {
    currentMaxTextures_ = maxTextures;
    currentMaxSamplers_ = maxSamplers;
   
    if (descriptorSetLayout_ != VK_NULL_HANDLE) {
        eng_.deferredTask(std::packaged_task<void()>(
            [device = eng_.vulkanDevice_.get()->getLogicalDevice(), dsl = descriptorSetLayout_] {
                vkDestroyDescriptorSetLayout(device, dsl, nullptr);
            }
        ), lastSubmitHandle);
    }
    if (descriptorPool_ != VK_NULL_HANDLE) {
        eng_.deferredTask(std::packaged_task<void()>(
            [device = eng_.vulkanDevice_.get()->getLogicalDevice(), dp = descriptorPool_] {
                vkDestroyDescriptorPool(device, dp, nullptr);
            }
        ), lastSubmitHandle);
    }

    VkShaderStageFlags stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT |
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT |
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT |
        VK_SHADER_STAGE_FRAGMENT_BIT |
        VK_SHADER_STAGE_COMPUTE_BIT;
    const VkDescriptorSetLayoutBinding
        bindings[kBinding_NumBindings] = {
          getDSLBinding(kBinding_Textures,
            VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTextures, stageFlags, nullptr),
          getDSLBinding(kBinding_Samplers,
            VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers, stageFlags, nullptr),
          getDSLBinding(kBinding_StorageImages,
            VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxTextures, stageFlags, nullptr),
    };
    const uint32_t flags =
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT |
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
    VkDescriptorBindingFlags bindingFlags[kBinding_NumBindings];
    for (int i = 0; i < kBinding_NumBindings; ++i) {
        bindingFlags[i] = flags;
    }
    const VkDescriptorSetLayoutBindingFlagsCreateInfo
        setLayoutBindingFlagsCI = {
          .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
          .bindingCount = kBinding_NumBindings,
          .pBindingFlags = bindingFlags,
    };
    const VkDescriptorSetLayoutCreateInfo dslci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .pNext = &setLayoutBindingFlagsCI,
      .flags =
        VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT,
      .bindingCount = kBinding_NumBindings,
      .pBindings = bindings,
    };
    ASSERT_VK_RESULT(vkCreateDescriptorSetLayout(eng_.vulkanDevice_.get()->getLogicalDevice(), &dslci, nullptr, &descriptorSetLayout_), "Creating descriptorSetLayout");

    const VkDescriptorPoolSize poolSizes[kBinding_NumBindings]{
    VkDescriptorPoolSize{
      VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxTextures},
    VkDescriptorPoolSize{
      VK_DESCRIPTOR_TYPE_SAMPLER, maxSamplers},
    VkDescriptorPoolSize{
      VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxTextures},
    };
    const VkDescriptorPoolCreateInfo ci = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
      .maxSets = 1,
      .poolSizeCount = kBinding_NumBindings,
      .pPoolSizes = poolSizes,
    };
    ASSERT_VK_RESULT(vkCreateDescriptorPool(eng_.vulkanDevice_.get()->getLogicalDevice(), &ci, nullptr, &descriptorPool_), "Creating descriptorPool");
    
    const VkDescriptorSetAllocateInfo ai = {
     .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
     .descriptorPool = descriptorPool_,
     .descriptorSetCount = 1,
     .pSetLayouts = &descriptorSetLayout_,
    };
    ASSERT_VK_RESULT(vkAllocateDescriptorSets(eng_.vulkanDevice_.get()->getLogicalDevice(), &ai, &descriptorSet_), "Allocating descriptorSet");
    return Result{};
}

void DescriptorManager::updateDescriptorSets(CommandManager* commandManager) {
    uint32_t newMaxTextures = currentMaxTextures_;
    uint32_t newMaxSamplers = currentMaxSamplers_;
    while (eng_.texturesPool_.objects_.size() > newMaxTextures) {
        newMaxTextures *= 2;
    }
    while (eng_.samplersPool_.objects_.size() > newMaxSamplers) {
        newMaxSamplers *= 2;
    }
    
    if (newMaxTextures != currentMaxTextures_ ||
        newMaxSamplers != currentMaxSamplers_) {
        growDescriptorPool(newMaxTextures, newMaxSamplers);
    }
    std::vector<VkDescriptorImageInfo> infoSampledImages;
    std::vector<VkDescriptorImageInfo> infoStorageImages;
    infoSampledImages.reserve(eng_.texturesPool_.numObjects());
    infoStorageImages.reserve(eng_.texturesPool_.numObjects());
    VkImageView dummyImageView = eng_.texturesPool_.objects_[0].obj_.getVkImageView();
    for (const auto& obj : eng_.texturesPool_.objects_) {
        const TextureManager tex = obj.obj_;
        const VkImageView view = obj.obj_.getVkImageView();
        const VkImageView storageView = obj.obj_.getVkImageViewStorage() ?
            obj.obj_.getVkImageViewStorage() : view;
        const bool isTextureAvailable = VK_SAMPLE_COUNT_1_BIT ==
            (tex.getSamples() & VK_SAMPLE_COUNT_1_BIT);
        const bool isSampledImage =
            isTextureAvailable && tex.isSampledImage();
        const bool isStorageImage =
            isTextureAvailable && tex.isStorageImage();
        infoSampledImages.push_back(VkDescriptorImageInfo{
          .sampler = VK_NULL_HANDLE,
          .imageView = isSampledImage ? view : dummyImageView,
          .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });
        infoStorageImages.push_back(VkDescriptorImageInfo{
          .sampler = VK_NULL_HANDLE,
          .imageView = isStorageImage ? storageView : dummyImageView,
          .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
        });
    }
    std::vector<VkDescriptorImageInfo> infoSamplers;
    infoSamplers.reserve(eng_.samplersPool_.objects_.size());
    for (const auto& sampler : eng_.samplersPool_.objects_) {
        infoSamplers.push_back({
          .sampler = sampler.obj_ ?
            sampler.obj_ : eng_.samplersPool_.objects_[0].obj_,
          .imageView = VK_NULL_HANDLE,
          .imageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        });
    }
    VkWriteDescriptorSet write[kBinding_NumBindings] = {};
    uint32_t numWrites = 0;
    if (!infoSampledImages.empty())
        write[numWrites++] = VkWriteDescriptorSet{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptorSet_,
          .dstBinding = kBinding_Textures,
          .dstArrayElement = 0,
          .descriptorCount = (uint32_t)infoSampledImages.size(),
          .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
          .pImageInfo = infoSampledImages.data(),
    };
    if (!infoSamplers.empty())
        write[numWrites++] = VkWriteDescriptorSet{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptorSet_,
          .dstBinding = kBinding_Samplers,
          .dstArrayElement = 0,
          .descriptorCount = (uint32_t)infoSamplers.size(),
          .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
          .pImageInfo = infoSamplers.data(),
    };
    if (!infoStorageImages.empty())
        write[numWrites++] = VkWriteDescriptorSet{
          .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
          .dstSet = descriptorSet_,
          .dstBinding = kBinding_StorageImages,
          .dstArrayElement = 0,
          .descriptorCount = (uint32_t)infoStorageImages.size(),
          .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
          .pImageInfo = infoStorageImages.data(),
    };
    if (numWrites) {
        commandManager->wait(commandManager->getLastSubmitHandle());
        vkUpdateDescriptorSets(eng_.vulkanDevice_.get()->getLogicalDevice(), numWrites, write, 0, nullptr);
    }
}

VkDescriptorSetLayoutBinding DescriptorManager::getDSLBinding(uint32_t binding,
    VkDescriptorType descriptorType,
    uint32_t descriptorCount,
    VkShaderStageFlags stageFlags,
    const VkSampler* immutableSamplers) {
    return VkDescriptorSetLayoutBinding{
        .binding = binding,
        .descriptorType = descriptorType,
        .descriptorCount = descriptorCount,
        .stageFlags = stageFlags,
        .pImmutableSamplers = immutableSamplers,
    };
}