#include "../CommandBuffer.h"
#include "../utils/Utils.h"
#include "../core/VulkanEngine.h"
#include "../core/VulkanDevice.h"
#include "../rendering/CommandManager.h"
#include "../rendering/VulkanGraphicsPipelineV2.h"
#include "../resources/BufferManager.h"
#include "../resources/TextureManager.h"

class VulkanEngine;
class VulkanDevice;
class VulkanGraphicsPipelineV2;
class BufferManager;

CommandBuffer::CommandBuffer(VulkanEngine* eng) {
    eng_ = eng; 
    wrapper_ = &eng_->commandManager_.get()->acquire();
}

CommandBuffer::~CommandBuffer() {
    // did you forget to call cmdEndRendering()?
    VK_ASSERT(!isRendering_);
}

void CommandBuffer::transitionToShaderReadOnly(TextureHandle handle) const {

    const TextureManager& img = *eng_->texturesPool_.get(handle);

    VK_ASSERT(!img.getIsSwapchainImage());

    // transition only non-multisampled images - MSAA images cannot be accessed from shaders
    if (img.getSamples() == VK_SAMPLE_COUNT_1_BIT) {
        const VkImageAspectFlags flags = img.getImageAspectFlags();
        // set the result of the previous render pass
        img.transitionLayout(wrapper_->cmdBuf_,
            img.isSampledImage() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_GENERAL,
            VkImageSubresourceRange{ flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
    }
}

//void CommandBuffer::cmdBindRayTracingPipeline(RayTracingPipelineHandle handle) {
//
//    if (!VK_VERIFY(!handle.empty() && eng_->has_KHR_ray_tracing_pipeline_)) {
//        return;
//    }
//
//    currentPipelineGraphics_ = {};
//    currentPipelineCompute_ = {};
//    currentPipelineRayTracing_ = handle;
//
//    VkPipeline pipeline = eng_->getVkPipeline(handle);
//
//    const RayTracingPipelineState* rtps = eng_->rayTracingPipelinesPool_.get(handle);
//
//    VK_ASSERT(rtps);
//    VK_ASSERT(pipeline != VK_NULL_HANDLE);
//
//    if (lastPipelineBound_ != pipeline) {
//        lastPipelineBound_ = pipeline;
//        vkCmdBindPipeline(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipeline);
//        eng_->checkAndUpdateDescriptorSets();
//        eng_->bindDefaultDescriptorSets(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rtps->pipelineLayout_);
//    }
//}

void CommandBuffer::cmdDispatchThreadGroups(const Dimensions& threadgroupCount, const Dependencies& deps) {
    VK_ASSERT(!isRendering_);

    for (uint32_t i = 0; i != Dependencies::LVK_MAX_SUBMIT_DEPENDENCIES && deps.textures[i]; i++) {
        //useComputeTexture(deps.textures[i], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    }
    for (uint32_t i = 0; i != Dependencies::LVK_MAX_SUBMIT_DEPENDENCIES && deps.buffers[i]; i++) {
        const BufferManager* buf = eng_->buffersPool_.get(deps.buffers[i]);
        bufferBarrier(deps.buffers[i],
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);
    }

    vkCmdDispatch(wrapper_->cmdBuf_, threadgroupCount.width, threadgroupCount.height, threadgroupCount.depth);
}

void CommandBuffer::cmdPushDebugGroupLabel(const char* label, uint32_t colorRGBA) const {
    VK_ASSERT(label);

    if (!label || !vkCmdBeginDebugUtilsLabelEXT) {
        return;
    }
    const VkDebugUtilsLabelEXT utilsLabel = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = {float((colorRGBA >> 0) & 0xff) / 255.0f,
                  float((colorRGBA >> 8) & 0xff) / 255.0f,
                  float((colorRGBA >> 16) & 0xff) / 255.0f,
                  float((colorRGBA >> 24) & 0xff) / 255.0f},
    };
    vkCmdBeginDebugUtilsLabelEXT(wrapper_->cmdBuf_, &utilsLabel);
}

void CommandBuffer::cmdInsertDebugEventLabel(const char* label, uint32_t colorRGBA) const {
    VK_ASSERT(label);

    if (!label || !vkCmdInsertDebugUtilsLabelEXT) {
        return;
    }
    const VkDebugUtilsLabelEXT utilsLabel = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = label,
        .color = {float((colorRGBA >> 0) & 0xff) / 255.0f,
                  float((colorRGBA >> 8) & 0xff) / 255.0f,
                  float((colorRGBA >> 16) & 0xff) / 255.0f,
                  float((colorRGBA >> 24) & 0xff) / 255.0f},
    };
    vkCmdInsertDebugUtilsLabelEXT(wrapper_->cmdBuf_, &utilsLabel);
}

void CommandBuffer::cmdPopDebugGroupLabel() const {
    if (!vkCmdEndDebugUtilsLabelEXT) {
        return;
    }
    vkCmdEndDebugUtilsLabelEXT(wrapper_->cmdBuf_);
}

//void CommandBuffer::useComputeTexture(TextureHandle handle, VkPipelineStageFlags2 dstStage) {
//
//    VK_ASSERT(!handle.empty());
//    TextureManager& tex = *eng_->texturesPool_.get(handle);
//
//    (void)dstStage; // TODO: add extra dstStage
//
//    if (!tex.isStorageImage() && !tex.isSampledImage()) {
//        VK_ASSERT_MSG(false, "Did you forget to specify TextureUsageBits::Storage or TextureUsageBits::Sampled on your texture?");
//        return;
//    }
//
//    tex.transitionLayout(wrapper_->cmdBuf_,
//        tex.isStorageImage() ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
//        VkImageSubresourceRange{ tex.getImageAspectFlags(), 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
//}

void CommandBuffer::bufferBarrier(BufferHandle handle, VkPipelineStageFlags2 srcStage, VkPipelineStageFlags2 dstStage) {

    BufferManager* buf = eng_->buffersPool_.get(handle);

    VkBufferMemoryBarrier2 barrier = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
        .srcStageMask = srcStage,
        .srcAccessMask = 0,
        .dstStageMask = dstStage,
        .dstAccessMask = 0,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .buffer = buf->vkBuffer_,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };

    if (srcStage & VK_PIPELINE_STAGE_2_TRANSFER_BIT) {
        barrier.srcAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    else {
        barrier.srcAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    }

    if (dstStage & VK_PIPELINE_STAGE_2_TRANSFER_BIT) {
        barrier.dstAccessMask |= VK_ACCESS_2_TRANSFER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT;
    }
    else {
        barrier.dstAccessMask |= VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT;
    }
    if (dstStage & VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT) {
        barrier.dstAccessMask |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
    }
    if (buf->getUsageFlags() & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
        barrier.dstAccessMask |= VK_ACCESS_2_INDEX_READ_BIT;
    }

    const VkDependencyInfo depInfo = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = 1,
        .pBufferMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(wrapper_->cmdBuf_, &depInfo);
}

void CommandBuffer::cmdBeginRendering(const RenderDesc& renderPass, const Framebuffer& fb, const Dependencies& deps) {

    VK_ASSERT(!isRendering_);

    isRendering_ = true;
    viewMask_ = renderPass.viewMask;

    for (uint32_t i = 0; i != Dependencies::LVK_MAX_SUBMIT_DEPENDENCIES && deps.textures[i]; i++) {
        transitionToShaderReadOnly(deps.textures[i]);
    }
    for (uint32_t i = 0; i != Dependencies::LVK_MAX_SUBMIT_DEPENDENCIES && deps.buffers[i]; i++) {
        VkPipelineStageFlags2 dstStageFlags = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        const BufferManager* buf = eng_->buffersPool_.get(deps.buffers[i]);
        VK_ASSERT(buf);
        if ((buf->getUsageFlags() & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) || (buf->getUsageFlags() & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)) {
            dstStageFlags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
        }
        if (buf->getUsageFlags() & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
            dstStageFlags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
        }
        bufferBarrier(deps.buffers[i], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, dstStageFlags);
    }

    const uint32_t numFbColorAttachments = fb.getNumColorAttachments();
    const uint32_t numPassColorAttachments = renderPass.getNumColorAttachments();

    VK_ASSERT(numPassColorAttachments == numFbColorAttachments);

    framebuffer_ = fb;

    // transition all the color attachments
    for (uint32_t i = 0; i != numFbColorAttachments; i++) {
        if (TextureHandle handle = fb.color[i].texture) {
            TextureManager* colorTex = eng_->texturesPool_.get(handle);
            transitionToColorAttachment(wrapper_->cmdBuf_, colorTex);
        }
        // handle MSAA
        /*if (TextureHandle handle = fb.color[i].resolveTexture) {
            TextureManager* colorResolveTex = eng_->texturesPool_.get(handle);
            colorResolveTex->isResolveAttachment = true;
            transitionToColorAttachment(wrapper_->cmdBuf_, colorResolveTex);
        }*/
    }
    // transition depth-stencil attachment
    TextureHandle depthTex = fb.depthStencil.texture;
    if (depthTex) {
        const TextureManager& depthImg = *eng_->texturesPool_.get(depthTex);
        const VkImageAspectFlags flags = depthImg.getImageAspectFlags();
        depthImg.transitionLayout(wrapper_->cmdBuf_,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VkImageSubresourceRange{ flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
    }
    // handle depth MSAA
    /*if (TextureHandle handle = fb.depthStencil.resolveTexture) {
        TextureManager& depthResolveImg = *eng_->texturesPool_.get(handle);
        depthResolveImg.isResolveAttachment = true;
        const VkImageAspectFlags flags = depthResolveImg.getImageAspectFlags();
        depthResolveImg.transitionLayout(wrapper_->cmdBuf_,
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VkImageSubresourceRange{ flags, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
    }*/

    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t mipLevel = 0;
    uint32_t fbWidth = 0;
    uint32_t fbHeight = 0;

    VkRenderingAttachmentInfo colorAttachments[VK_MAX_COLOR_ATTACHMENTS];

    for (uint32_t i = 0; i != numFbColorAttachments; i++) {
        const Framebuffer::AttachmentDesc& attachment = fb.color[i];
        VK_ASSERT(!attachment.texture.empty());

        TextureManager& colorTexture = *eng_->texturesPool_.get(attachment.texture);
        const RenderDesc::AttachmentDesc& descColor = renderPass.color[i];
        if (mipLevel && descColor.level) {
            VK_ASSERT_MSG(descColor.level == mipLevel, "All color attachments should have the same mip-level");
        }
        const VkExtent3D dim = colorTexture.getExtent();
        if (fbWidth) {
            VK_ASSERT_MSG(dim.width == fbWidth, "All attachments should have the same width");
        }
        if (fbHeight) {
            VK_ASSERT_MSG(dim.height == fbHeight, "All attachments should have the same height");
        }
        mipLevel = descColor.level;
        fbWidth = dim.width;
        fbHeight = dim.height;
        samples = colorTexture.getSamples();
        colorAttachments[i] = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = colorTexture.getOrCreateVkImageViewForFramebuffer(*eng_, descColor.level, descColor.layer),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            //.resolveMode = (samples > 1) ? resolveModeToVkResolveModeFlagBits(descColor.resolveMode, VK_RESOLVE_MODE_FLAG_BITS_MAX_ENUM)
                                        // : VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = loadOpToVkAttachmentLoadOp(descColor.loadOp),
            .storeOp = storeOpToVkAttachmentStoreOp(descColor.storeOp),
        };
        memcpy(&colorAttachments[i].clearValue.color, &descColor.clearColor, sizeof(descColor.clearColor));
        // handle MSAA
        if (descColor.storeOp == StoreOp_MsaaResolve) {
            VK_ASSERT(samples > 1);
            VK_ASSERT_MSG(!attachment.resolveTexture.empty(), "Framebuffer attachment should contain a resolve texture");
            TextureManager& colorResolveTexture = *eng_->texturesPool_.get(attachment.resolveTexture);
            colorAttachments[i].resolveImageView =
                colorResolveTexture.getOrCreateVkImageViewForFramebuffer(*eng_, descColor.level, descColor.layer);
            colorAttachments[i].resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
    }

    VkRenderingAttachmentInfo depthAttachment = {};

    if (fb.depthStencil.texture) {
        TextureManager& depthTexture = *eng_->texturesPool_.get(fb.depthStencil.texture);
        const RenderDesc::AttachmentDesc& descDepth = renderPass.depth;
        VK_ASSERT_MSG(descDepth.level == mipLevel, "Depth attachment should have the same mip-level as color attachments");
        depthAttachment = {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = depthTexture.getOrCreateVkImageViewForFramebuffer(*eng_, descDepth.level, descDepth.layer),
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = loadOpToVkAttachmentLoadOp(descDepth.loadOp),
            .storeOp = storeOpToVkAttachmentStoreOp(descDepth.storeOp),
            .clearValue = {.depthStencil = {.depth = descDepth.clearDepth, .stencil = descDepth.clearStencil}},
        };
        // handle depth MSAA
        if (descDepth.storeOp == StoreOp_MsaaResolve) {
            VK_ASSERT(depthTexture.getSamples() == samples);
            const Framebuffer::AttachmentDesc& attachment = fb.depthStencil;
            VK_ASSERT_MSG(!attachment.resolveTexture.empty(), "Framebuffer depth attachment should contain a resolve texture");
            TextureManager& depthResolveTexture = *eng_->texturesPool_.get(attachment.resolveTexture);
            depthAttachment.resolveImageView = depthResolveTexture.getOrCreateVkImageViewForFramebuffer(*eng_, descDepth.level, descDepth.layer);
            depthAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            /*depthAttachment.resolveMode =
                resolveModeToVkResolveModeFlagBits(descDepth.resolveMode, eng_->vkPhysicalDeviceVulkan12Properties_.supportedDepthResolveModes);*/
        }
        const VkExtent3D dim = depthTexture.getExtent();
        if (fbWidth) {
            VK_ASSERT_MSG(dim.width == fbWidth, "All attachments should have the same width");
        }
        if (fbHeight) {
            VK_ASSERT_MSG(dim.height == fbHeight, "All attachments should have the same height");
        }
        mipLevel = descDepth.level;
        fbWidth = dim.width;
        fbHeight = dim.height;
    }

    const uint32_t width = std::max(fbWidth >> mipLevel, 1u);
    const uint32_t height = std::max(fbHeight >> mipLevel, 1u);
    const Viewport viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, +1.0f };
    const ScissorRect scissor = { 0, 0, width, height };

    VkRenderingAttachmentInfo stencilAttachment = depthAttachment;

    const bool isStencilFormat = renderPass.stencil.loadOp != LoadOp_Invalid;

    const VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .pNext = nullptr,
        .flags = 0,
        .renderArea = {VkOffset2D{(int32_t)scissor.x, (int32_t)scissor.y}, VkExtent2D{scissor.width, scissor.height}},
        .layerCount = renderPass.layerCount,
        .viewMask = renderPass.viewMask,
        .colorAttachmentCount = numFbColorAttachments,
        .pColorAttachments = colorAttachments,
        .pDepthAttachment = depthTex ? &depthAttachment : nullptr,
        .pStencilAttachment = isStencilFormat ? &stencilAttachment : nullptr,
    };

    cmdBindViewport(viewport);
    cmdBindScissorRect(scissor);
    cmdBindDepthState({});

    eng_->checkAndUpdateDescriptorSets();

    vkCmdSetDepthCompareOp(wrapper_->cmdBuf_, VK_COMPARE_OP_ALWAYS);
    vkCmdSetDepthBiasEnable(wrapper_->cmdBuf_, VK_FALSE);

    vkCmdBeginRendering(wrapper_->cmdBuf_, &renderingInfo);
}

void CommandBuffer::cmdEndRendering() {
    VK_ASSERT(isRendering_);

    isRendering_ = false;

    vkCmdEndRendering(wrapper_->cmdBuf_);

    framebuffer_ = {};
}

void CommandBuffer::cmdBindViewport(const Viewport& viewport) {
    // https://www.saschawillems.de/blog/2019/03/29/flipping-the-vulkan-viewport/
    const VkViewport vp = {
        .x = viewport.x, // float x;
        .y = viewport.height - viewport.y, // float y;
        .width = viewport.width, // float width;
        .height = -viewport.height, // float height;
        .minDepth = viewport.minDepth, // float minDepth;
        .maxDepth = viewport.maxDepth, // float maxDepth;
    };
    vkCmdSetViewport(wrapper_->cmdBuf_, 0, 1, &vp);
}

void CommandBuffer::cmdBindScissorRect(const ScissorRect& rect) {
    const VkRect2D scissor = {
        VkOffset2D{(int32_t)rect.x, (int32_t)rect.y},
        VkExtent2D{rect.width, rect.height},
    };
    vkCmdSetScissor(wrapper_->cmdBuf_, 0, 1, &scissor);
}

void CommandBuffer::cmdBindRenderPipeline(RenderPipelineHandle handle) {
    if (!VK_VERIFY(!handle.empty())) {
        return;
    }

    currentPipelineGraphics_ = handle;
    //currentPipelineCompute_ = {};
    //currentPipelineRayTracing_ = {};

    const VulkanGraphicsPipelineV2* pipe = eng_->renderPipelinesPool_.get(handle);

    VK_ASSERT(pipe);

    const bool hasDepthAttachmentPipeline = pipe->getDesc().depthFormat != VK_FORMAT_UNDEFINED;
    const bool hasDepthAttachmentPass = !framebuffer_.depthStencil.texture.empty();

    if (hasDepthAttachmentPipeline != hasDepthAttachmentPass) {
        VK_ASSERT(false);
        printf("Make sure your render pass and render pipeline both have matching depth attachments");
    }

    VkPipeline pipeline = eng_->getVkPipeline(handle, viewMask_);

    VK_ASSERT(pipeline != VK_NULL_HANDLE);

    if (lastPipelineBound_ != pipeline) {
        lastPipelineBound_ = pipeline;
        vkCmdBindPipeline(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
        eng_->bindDefaultDescriptorSets(wrapper_->cmdBuf_, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe->getPipelineLayout());
    }
}

void CommandBuffer::cmdBindDepthState(const DepthState& desc) {

    const VkCompareOp op = desc.compareOp;
    vkCmdSetDepthWriteEnable(wrapper_->cmdBuf_, desc.isDepthWriteEnabled ? VK_TRUE : VK_FALSE);
    vkCmdSetDepthTestEnable(wrapper_->cmdBuf_, (op != VK_COMPARE_OP_ALWAYS || desc.isDepthWriteEnabled) ? VK_TRUE : VK_FALSE);

#if defined(ANDROID)
    // This is a workaround for the issue.
    // On Android (Mali-G715-Immortalis MC11 v1.r38p1-01eac0.c1a71ccca2acf211eb87c5db5322f569)
    // if depth-stencil texture is not set, call of vkCmdSetDepthCompareOp leads to disappearing of all content.
    if (!framebuffer_.depthStencil.texture) {
        return;
    }
#endif
    vkCmdSetDepthCompareOp(wrapper_->cmdBuf_, op);
}

void CommandBuffer::cmdBindVertexBuffer(uint32_t index, BufferHandle buffer, uint64_t bufferOffset) {

    if (!VK_VERIFY(!buffer.empty())) {
        return;
    }

    BufferManager* buf = eng_->buffersPool_.get(buffer);

    VK_ASSERT(buf->getUsageFlags() & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    vkCmdBindVertexBuffers2(wrapper_->cmdBuf_, index, 1, &buf->vkBuffer_, &bufferOffset, nullptr, nullptr);
}

void CommandBuffer::cmdBindIndexBuffer(BufferHandle indexBuffer, IndexFormat_e indexFormat, uint64_t indexBufferOffset) {
    BufferManager* buf = eng_->buffersPool_.get(indexBuffer);

    VK_ASSERT(buf->getUsageFlags() & VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

    const VkIndexType type = indexFormatToVkIndexType(indexFormat);
    vkCmdBindIndexBuffer(wrapper_->cmdBuf_, buf->vkBuffer_, indexBufferOffset, type);
}

void CommandBuffer::cmdPushConstants(const void* data, size_t size, size_t offset) {

    VK_ASSERT(size % 4 == 0); // VUID-vkCmdPushConstants-size-00369: size must be a multiple of 4

    // check push constant size is within max size
    
    const VkPhysicalDeviceLimits& limits = eng_->vulkanDevice_.get()->getPhysicalDeviceProperties().limits;
    if (!VK_VERIFY(size + offset <= limits.maxPushConstantsSize)) {
        printf("Push constants size exceeded %u (max %u bytes)", size + offset, limits.maxPushConstantsSize);
    }

    if (currentPipelineGraphics_.empty()) {
        return;
    }

    const VulkanGraphicsPipelineV2* stateGraphics = eng_->renderPipelinesPool_.get(currentPipelineGraphics_);
    //const ComputePipelineState* stateCompute = eng_->computePipelinesPool_.get(currentPipelineCompute_);
    //const RayTracingPipelineState* stateRayTracing = eng_->rayTracingPipelinesPool_.get(currentPipelineRayTracing_);

    VK_ASSERT(stateGraphics);

    VkPipelineLayout layout = stateGraphics->getPipelineLayout();
    VkShaderStageFlags shaderStageFlags = stateGraphics->getShaderStageFlags();

    vkCmdPushConstants(wrapper_->cmdBuf_, layout, shaderStageFlags, (uint32_t)offset, (uint32_t)size, data);
}

void CommandBuffer::cmdFillBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, uint32_t data) {
    VK_ASSERT(buffer.valid());
    VK_ASSERT(size);
    VK_ASSERT(size % 4 == 0);
    VK_ASSERT(bufferOffset % 4 == 0);

    BufferManager* buf = eng_->buffersPool_.get(buffer);

    bufferBarrier(buffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    vkCmdFillBuffer(wrapper_->cmdBuf_, buf->vkBuffer_, bufferOffset, size, data);

    VkPipelineStageFlags2 dstStage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

    if (buf->getUsageFlags() & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
        dstStage |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    }
    if (buf->getUsageFlags() & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
        dstStage |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
    }

    bufferBarrier(buffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, dstStage);
}

void CommandBuffer::cmdUpdateBuffer(BufferHandle buffer, size_t bufferOffset, size_t size, const void* data) {
    VK_ASSERT(buffer.valid());
    VK_ASSERT(data);
    VK_ASSERT(size && size <= 65536);
    VK_ASSERT(size % 4 == 0);
    VK_ASSERT(bufferOffset % 4 == 0);

    BufferManager* buf = eng_->buffersPool_.get(buffer);

    bufferBarrier(buffer, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

    vkCmdUpdateBuffer(wrapper_->cmdBuf_, buf->vkBuffer_, bufferOffset, size, data);

    VkPipelineStageFlags2 dstStage = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;

    if (buf->getUsageFlags() & VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT) {
        dstStage |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
    }
    if (buf->getUsageFlags() & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
        dstStage |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
    }

    bufferBarrier(buffer, VK_PIPELINE_STAGE_2_TRANSFER_BIT, dstStage);
}

void CommandBuffer::cmdDraw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t baseInstance) {

    if (vertexCount == 0) {
        return;
    }

    vkCmdDraw(wrapper_->cmdBuf_, vertexCount, instanceCount, firstVertex, baseInstance);
}

void CommandBuffer::cmdDrawIndexed(uint32_t indexCount,
    uint32_t instanceCount,
    uint32_t firstIndex,
    int32_t vertexOffset,
    uint32_t baseInstance) {

    if (indexCount == 0) {
        return;
    }

    vkCmdDrawIndexed(wrapper_->cmdBuf_, indexCount, instanceCount, firstIndex, vertexOffset, baseInstance);
}

void CommandBuffer::cmdDrawIndirect(BufferHandle indirectBuffer, size_t indirectBufferOffset, uint32_t drawCount, uint32_t stride) {

    BufferManager* bufIndirect = eng_->buffersPool_.get(indirectBuffer);

    VK_ASSERT(bufIndirect);

    vkCmdDrawIndirect(
        wrapper_->cmdBuf_, bufIndirect->vkBuffer_, indirectBufferOffset, drawCount, stride ? stride : sizeof(VkDrawIndirectCommand));
}

void CommandBuffer::cmdDrawIndexedIndirect(BufferHandle indirectBuffer,
    size_t indirectBufferOffset,
    uint32_t drawCount,
    uint32_t stride) {

    BufferManager* bufIndirect = eng_->buffersPool_.get(indirectBuffer);

    VK_ASSERT(bufIndirect);

    vkCmdDrawIndexedIndirect(
        wrapper_->cmdBuf_, bufIndirect->vkBuffer_, indirectBufferOffset, drawCount, stride ? stride : sizeof(VkDrawIndexedIndirectCommand));
}

void CommandBuffer::cmdDrawIndexedIndirectCount(BufferHandle indirectBuffer,
    size_t indirectBufferOffset,
    BufferHandle countBuffer,
    size_t countBufferOffset,
    uint32_t maxDrawCount,
    uint32_t stride) {

    BufferManager* bufIndirect = eng_->buffersPool_.get(indirectBuffer);
    BufferManager* bufCount = eng_->buffersPool_.get(countBuffer);

    VK_ASSERT(bufIndirect);
    VK_ASSERT(bufCount);

    vkCmdDrawIndexedIndirectCount(wrapper_->cmdBuf_,
        bufIndirect->vkBuffer_,
        indirectBufferOffset,
        bufCount->vkBuffer_,
        countBufferOffset,
        maxDrawCount,
        stride ? stride : sizeof(VkDrawIndexedIndirectCommand));
}


void CommandBuffer::cmdSetBlendColor(const float color[4]) {
    vkCmdSetBlendConstants(wrapper_->cmdBuf_, color);
}

void CommandBuffer::cmdSetDepthBias(float constantFactor, float slopeFactor, float clamp) {
    vkCmdSetDepthBias(wrapper_->cmdBuf_, constantFactor, clamp, slopeFactor);
}

void CommandBuffer::cmdSetDepthBiasEnable(bool enable) {
    vkCmdSetDepthBiasEnable(wrapper_->cmdBuf_, enable ? VK_TRUE : VK_FALSE);
}

void CommandBuffer::cmdResetQueryPool(QueryPoolHandle pool, uint32_t firstQuery, uint32_t queryCount) {
    VkQueryPool vkPool = *eng_->queriesPool_.get(pool);

    vkCmdResetQueryPool(wrapper_->cmdBuf_, vkPool, firstQuery, queryCount);
}

void CommandBuffer::cmdWriteTimestamp(QueryPoolHandle pool, uint32_t query) {
    VkQueryPool vkPool = *eng_->queriesPool_.get(pool);

    vkCmdWriteTimestamp(wrapper_->cmdBuf_, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkPool, query);
}

void CommandBuffer::cmdClearColorImage(TextureHandle tex, const VkClearColorValue& value, const TextureLayers& layers) {

    TextureManager* img = eng_->texturesPool_.get(tex);

    if (!VK_VERIFY(img)) {
        return;
    }

    const VkImageSubresourceRange range = {
        .aspectMask = img->getImageAspectFlags(),
        .baseMipLevel = layers.mipLevel,
        .levelCount = VK_REMAINING_MIP_LEVELS,
        .baseArrayLayer = layers.layer,
        .layerCount = layers.numLayers,
    };

    imageMemoryBarrier(
        wrapper_->cmdBuf_,
        img->getVkImage(),
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
        img->getCurrentLayout(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        range);

    vkCmdClearColorImage(wrapper_->cmdBuf_,
        img->getVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        reinterpret_cast<const VkClearColorValue*>(&value),
        1,
        &range);

    // a ternary cascade...
    const VkImageLayout newLayout = img->getCurrentLayout() == VK_IMAGE_LAYOUT_UNDEFINED
        ? (img->isAttachment() ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
            : img->isSampledImage() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_GENERAL)
        : img->getCurrentLayout();

    imageMemoryBarrier(
        wrapper_->cmdBuf_,
        img->getVkImage(),
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        newLayout,
        range);

    img->setCurrentLayout(newLayout);
}

void CommandBuffer::cmdCopyImage(TextureHandle src,
    TextureHandle dst,
    const Dimensions& extent,
    const VkOffset3D& srcOffset,
    const VkOffset3D& dstOffset,
    const TextureLayers& srcLayers,
    const TextureLayers& dstLayers) {

    TextureManager* imgSrc = eng_->texturesPool_.get(src);
    TextureManager* imgDst = eng_->texturesPool_.get(dst);

    VK_ASSERT(imgSrc && imgDst);
    VK_ASSERT(srcLayers.numLayers == dstLayers.numLayers);

    if (!imgSrc || !imgDst) {
        return;
    }

    const VkImageSubresourceRange rangeSrc = {
        .aspectMask = imgSrc->getImageAspectFlags(),
        .baseMipLevel = srcLayers.mipLevel,
        .levelCount = 1,
        .baseArrayLayer = srcLayers.layer,
        .layerCount = srcLayers.numLayers,
    };
    const VkImageSubresourceRange rangeDst = {
        .aspectMask = imgDst->getImageAspectFlags(),
        .baseMipLevel = dstLayers.mipLevel,
        .levelCount = 1,
        .baseArrayLayer = dstLayers.layer,
        .layerCount = dstLayers.numLayers,
    };

    VK_ASSERT(imgSrc->getCurrentLayout() != VK_IMAGE_LAYOUT_UNDEFINED);

    const VkExtent3D dstExtent = imgDst->getExtent();
    const bool coversFullDstImage = dstExtent.width == extent.width && dstExtent.height == extent.height && dstExtent.depth == extent.depth &&
        dstOffset.x == 0 && dstOffset.y == 0 && dstOffset.z == 0;

    VK_ASSERT(coversFullDstImage || imgDst->getCurrentLayout() != VK_IMAGE_LAYOUT_UNDEFINED);

    imageMemoryBarrier(
        wrapper_->cmdBuf_,
        imgSrc->getVkImage(),
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT },
        imgSrc->getCurrentLayout(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        rangeSrc);
    imageMemoryBarrier(
        wrapper_->cmdBuf_,
        imgDst->getVkImage(),
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
        coversFullDstImage ? VK_IMAGE_LAYOUT_UNDEFINED : imgDst->getCurrentLayout(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        rangeDst);

    const VkImageCopy regionCopy = {
        .srcSubresource =
            {
                .aspectMask = imgSrc->getImageAspectFlags(),
                .mipLevel = srcLayers.mipLevel,
                .baseArrayLayer = srcLayers.layer,
                .layerCount = srcLayers.numLayers,
            },
        .srcOffset = {.x = srcOffset.x, .y = srcOffset.y, .z = srcOffset.z},
        .dstSubresource =
            {
                .aspectMask = imgDst->getImageAspectFlags(),
                .mipLevel = dstLayers.mipLevel,
                .baseArrayLayer = dstLayers.layer,
                .layerCount = dstLayers.numLayers,
            },
        .dstOffset = {.x = dstOffset.x, .y = dstOffset.y, .z = dstOffset.z},
        .extent = {.width = extent.width, .height = extent.height, .depth = extent.depth},
    };
    const VkImageBlit regionBlit = {
        .srcSubresource = regionCopy.srcSubresource,
        .srcOffsets = {{},
                       {.x = int32_t(srcOffset.x + imgSrc->getExtent().width),
                        .y = int32_t(srcOffset.y + imgSrc->getExtent().height),
                        .z = int32_t(srcOffset.z + imgSrc->getExtent().depth)}},
        .dstSubresource = regionCopy.dstSubresource,
        .dstOffsets = {{},
                       {.x = int32_t(dstOffset.x + imgDst->getExtent().width),
                        .y = int32_t(dstOffset.y + imgDst->getExtent().height),
                        .z = int32_t(dstOffset.z + imgDst->getExtent().depth)}},
    };

    const bool isCompatible = getBytesPerPixel(imgSrc->getImageFormat()) == getBytesPerPixel(imgDst->getImageFormat());

    isCompatible ? vkCmdCopyImage(wrapper_->cmdBuf_,
        imgSrc->getVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        imgDst->getVkImage(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &regionCopy)
        : vkCmdBlitImage(wrapper_->cmdBuf_,
            imgSrc->getVkImage(),
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            imgDst->getVkImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1,
            &regionBlit,
            VK_FILTER_LINEAR);

    imageMemoryBarrier(
        wrapper_->cmdBuf_,
        imgSrc->getVkImage(),
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_READ_BIT },
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        imgSrc->getCurrentLayout(),
        rangeSrc);

    // a ternary cascade...
    const VkImageLayout newLayout = imgDst->getCurrentLayout() == VK_IMAGE_LAYOUT_UNDEFINED
        ? (imgDst->isAttachment() ? VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL
            : imgDst->isSampledImage() ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            : VK_IMAGE_LAYOUT_GENERAL)
        : imgDst->getCurrentLayout();

    imageMemoryBarrier(
        wrapper_->cmdBuf_,
        imgDst->getVkImage(),
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_TRANSFER_BIT, .access = VK_ACCESS_2_TRANSFER_WRITE_BIT },
        StageAccess{ .stage = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, .access = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT },
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        newLayout,
        rangeDst);

    imgDst->setCurrentLayout(newLayout);
}

void CommandBuffer::cmdGenerateMipmap(TextureHandle handle) {
    if (handle.empty()) {
        return;
    }

    const TextureManager* tex = eng_->texturesPool_.get(handle);

    if (tex->getNumLevels() <= 1) {
        return;
    }

    VK_ASSERT(tex->getCurrentLayout() != VK_IMAGE_LAYOUT_UNDEFINED);

    //tex->generateMipmap(wrapper_->cmdBuf_);
}
