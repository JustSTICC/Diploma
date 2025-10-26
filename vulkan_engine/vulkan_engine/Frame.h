#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <iostream>
class Frame
{
public:
	Frame(VkImage image, VkImageView imageView, VkFormat swapchainFormat);

	void setCommandBuffer(VkCommandBuffer commandBuffer, 
		std::vector<VkShaderEXT>& shaders,
		VkExtent2D frameSize);

	VkImage image;
	VkImageView imageView;
	VkCommandBuffer commandBuffer;
	
private:
	void buildRenderingInfo(vk::Extent2D frameSize);
	void buildColorAttachmentInfo();
	void buildDepthAttachmentInfo();
	vk::RenderingInfoKHR renderingInfo2;
	VkRenderingInfoKHR renderingInfo{};
	VkRenderingAttachmentInfoKHR colorAttachment{};
	VkRenderingAttachmentInfoKHR depthAttachment{};

};

