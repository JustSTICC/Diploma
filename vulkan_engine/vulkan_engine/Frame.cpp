#include "Frame.h"

Frame::Frame(VkImage image, VkImageView imageView, VkFormat swapchainFormat) {
	this->image = image;
	this->imageView = imageView;
}

void Frame::setCommandBuffer(VkCommandBuffer commandBuffer,
	std::vector<VkShaderEXT>& shaders,
	VkExtent2D frameSize) {

	this->commandBuffer = commandBuffer;

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    VkResult beginCommandBufferResult = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (beginCommandBufferResult != VK_SUCCESS) {
        std::cout << "failed to begin recording command buffer - " << beginCommandBufferResult << std::endl;
        throw std::runtime_error("failed to begin recording command buffer!");
    }
    else {
        std::cout << "Successfully started recording command buffer - " << beginCommandBufferResult << std::endl;
    }
}

void Frame::buildRenderingInfo(vk::Extent2D frameSize) {

    //renderingInfo2.setFlags(vk::RenderingFlagsKHR());
    //renderingInfo.setRenderArea(vk::Rect2D({ 0,0 }, frameSize));
    //renderingInfo.setLayerCount(1);
    ////bitmask indicating the layers which will be rendered to
    //renderingInfo.setViewMask(0);
    //renderingInfo.setColorAttachmentCount(1);
    //renderingInfo.setColorAttachments(colorAttachment);

    renderingInfo.flags = 0;
	renderingInfo.renderArea = vk::Rect2D({ 0,0 }, frameSize);
	renderingInfo.layerCount = 1;
	renderingInfo.viewMask = 0;
	renderingInfo.colorAttachmentCount = 1;
	renderingInfo.pColorAttachments = &colorAttachment;
}

void Frame::buildColorAttachmentInfo() {
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
    colorAttachment.pNext = nullptr;
    colorAttachment.imageView = imageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
    colorAttachment.resolveImageView = VK_NULL_HANDLE;
    colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

  /*  colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;*/
}

void Frame::buildDepthAttachmentInfo() {
    /*depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;*/

	depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
	depthAttachment.pNext = nullptr;
	depthAttachment.imageView = imageView; // to be filled when depth image is created

}

