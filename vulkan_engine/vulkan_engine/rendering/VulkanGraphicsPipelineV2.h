#pragma once
#include "../rendering/PipelineBuilder.h"
#include "../Shader.h"



class VulkanGraphicsPipelineV2 final{
public:
	VulkanGraphicsPipelineV2();
	~VulkanGraphicsPipelineV2();

    void createRenderPipeline(const PipelineDesc& pipeDesc);
    void cleanup();
    /*void recreate(const VulkanSwapchain& swapchain);*/
	void setPipeline(VkPipeline pipeline) { pipeline_ = pipeline; }
	void setLastDescriptorSetLayout(VkDescriptorSetLayout layout) { lastVkDescriptorSetLayout_ = layout; }
    void getVkPipeline(Pool<ShaderModule, Shader>& shaderModulesPool,
        VkDescriptorSetLayout vkDSL,
        VkDevice device,
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo,
        VkPhysicalDeviceLimits limits) const;

	PipelineDesc getDesc() const { return desc_; }
	VkPipeline getPipeline() const { return pipeline_; }
	VkPipelineLayout getPipelineLayout() const { return pipelineLayout_; }
	VkShaderStageFlags getShaderStageFlags() const { return shaderStageFlags_; }
	VkDescriptorSetLayout getLastDescriptorSetLayout() const { return lastVkDescriptorSetLayout_; }
    void* specConstantDataStorage_ = nullptr;
private:
    PipelineDesc desc_;
    uint32_t numBindings_ = 0;
    uint32_t numAttributes_ = 0;
    VkVertexInputBindingDescription vkBindings_[VK_VERTEX_BUFFER_MAX] = {};
    VkVertexInputAttributeDescription vkAttributes_[VK_VERTEX_ATTRIBUTES_MAX] = {};
    VkDescriptorSetLayout lastVkDescriptorSetLayout_ = VK_NULL_HANDLE;
    mutable VkShaderStageFlags shaderStageFlags_ = 0;

    VkPipelineCache pipelineCache_ = VK_NULL_HANDLE;
    mutable VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    mutable VkPipeline pipeline_ = VK_NULL_HANDLE;


 

    VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(VkShaderStageFlagBits stage,
        VkShaderModule shaderModule,
        const char* entryPoint,
        const VkSpecializationInfo* specializationInfo) const;
    //VkSampleCountFlagBits getVulkanSampleCountFlags(uint32_t numSamples, VkSampleCountFlags maxSamplesMask) const;
    VkSpecializationInfo getPipelineShaderStageSpecializationInfo(SpecializationConstantDesc desc, VkSpecializationMapEntry* outEntries) const;
};