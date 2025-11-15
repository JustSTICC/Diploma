#include "VulkanEngine.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "../Shader.h"
#include "../rendering/VulkanSwapchain.h"
#include "../rendering/VulkanGraphicsPipelineV2.h"
#include "../rendering/VulkanGraphicsPipeline.h"
#include "../rendering/CommandManager.h"
#include "../resources/BufferManager.h"
#include "../resources/TextureManager.h"
#include "../resources/StagingDevice.h"
#include "../descriptors/DescriptorManager.h"
#include "../ui/GuiManager.h"

#include "../utils/Utils.h"
#include "../utils/ScopeExit.h"
#include <stdexcept>
#include <iostream>
#include <memory>


VulkanEngine::VulkanEngine(const Config& config) : config_(config), window_(nullptr), surface_(VK_NULL_HANDLE) {}

VulkanEngine::~VulkanEngine(){
    cleanup();
}

void VulkanEngine::run(){
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void VulkanEngine::initWindow(){
    glfwSetErrorCallback([](int error,
        const char* description) {
            printf("GLFW Error (%i): %s\n", error, description);
        });
    if (!glfwInit()) {
        throw std::runtime_error("glfwInit failed!");
    }
    const bool wantsWholeArea = !config_.windowWidth || !config_.windowHeight;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, wantsWholeArea ? GLFW_FALSE : GLFW_TRUE);
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    int x = 0;
    int y = 0;
    int w = mode->width;
    int h = mode->height;
    if (wantsWholeArea) {
        glfwGetMonitorWorkarea(monitor, &x, &y, &w, &h);
    }
    else {
        w = config_.windowWidth;
        h = config_.windowHeight;
    }

    window_ = glfwCreateWindow(
        w, h, config_.windowTitle.c_str(), nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return;
    }
    if (wantsWholeArea) {
        glfwSetWindowPos(window_, x, y);
    }
    glfwGetWindowSize(window_, &w, &h);
    config_.windowWidth = (uint32_t)w;
    config_.windowHeight = (uint32_t)h;

    glfwSetKeyCallback(window_, [](GLFWwindow* window,
        int key, int, int action, int) {
            if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
        });
    glfwSetWindowUserPointer(window_, this);
    glfwSetFramebufferSizeCallback(window_, framebufferResizeCallback);
}

void VulkanEngine::initVulkan(){


    vulkanInstance_ = std::make_unique<VulkanInstance>();
    vulkanInstance_->initialize();

    if(glfwCreateWindowSurface(vulkanInstance_->getInstance(), window_, nullptr, &surface_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }

    vulkanDevice_ = std::make_unique<VulkanDevice>();
    vulkanDevice_->initialize(*vulkanInstance_, surface_);

    vulkanSwapchain_ = std::make_unique<VulkanSwapchain>();
    vulkanSwapchain_->initialize(*vulkanDevice_, surface_, window_);

    //vertShader_ = std::make_unique<Shader>();
    //vertShader_->initialize(vulkanDevice_->getLogicalDevice(), VERTEX_SHADER_PATH);

    //fragShader_ = std::make_unique<Shader>();
    //fragShader_->initialize(vulkanDevice_->getLogicalDevice(), FRAGMENT_SHADER_PATH);

    if (config_.maxFramesInFlight == NULL) {
		config_.maxFramesInFlight = vulkanSwapchain_->getImages().size();
    }

    //vulkanPipeline_ = std::make_unique<VulkanGraphicsPipeline>();
    //vulkanPipeline_->initialize(*vulkanDevice_, *vulkanSwapchain_);

    commandManager_ = std::make_unique<CommandManager>();
    commandManager_->initialize(*vulkanDevice_);

    //bufferManager_ = std::make_unique<BufferManager>();
    //bufferManager_->(*vulkanDevice_, *commandManager_);

    //textureManager_ = std::make_unique<TextureManager>();
    //textureManager_->initialize(*vulkanDevice_, *commandManager_, *bufferManager_);

    //descriptorManager_ = std::make_unique<DescriptorManager>();
    //descriptorManager_->initialize(*vulkanDevice_);

    // Initialize GUI if enabled
    //if (config_.enableGui) {
    //    GuiManager::Config guiConfig;
    //    guiConfig.maxFramesInFlight = config_.maxFramesInFlight;
    //    guiConfig.fontPath = config_.fontPath;
    //    guiConfig.fontSize = config_.fontSize;
    //    guiManager_ = std::make_unique<GuiManager>(guiConfig);
    //    guiManager_->initialize(window_, vulkanInstance_->getInstance(), 
    //                           vulkanDevice_.get(), vulkanSwapchain_.get(), 
    //                           vulkanPipeline_->getRenderPass());
    //}

    createSyncObjects();

    initializeResources();
}

void VulkanEngine::createSyncObjects() {
    imageAvailableSemaphores_.resize(config_.maxFramesInFlight);
    renderFinishedSemaphores_.resize(config_.maxFramesInFlight);
    inFlightFences_.resize(config_.maxFramesInFlight);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < config_.maxFramesInFlight; i++) {
        if (vkCreateSemaphore(vulkanDevice_->getLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores_[i]) != VK_SUCCESS ||
            vkCreateSemaphore(vulkanDevice_->getLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores_[i]) != VK_SUCCESS ||
            vkCreateFence(vulkanDevice_->getLogicalDevice(), &fenceInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void VulkanEngine::mainLoop() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(vulkanDevice_->getLogicalDevice());
}

void VulkanEngine::drawFrame() {
    vkWaitForFences(vulkanDevice_->getLogicalDevice(), 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);
    vkResetFences(vulkanDevice_->getLogicalDevice(), 1, &inFlightFences_[currentFrame_]);

    uint32_t imageIndex{ 0 };
    VkResult acquireNextImageResult = 
        vkAcquireNextImageKHR(vulkanDevice_->getLogicalDevice(), vulkanSwapchain_->getSwapChain(), UINT64_MAX, imageAvailableSemaphores_[currentFrame_], VK_NULL_HANDLE, &imageIndex);
    if (acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (acquireNextImageResult != VK_SUCCESS && acquireNextImageResult != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }
    vkResetFences(vulkanDevice_->getLogicalDevice(), 1, &inFlightFences_[currentFrame_]);

    updateUniforms(currentFrame_);

    // Start GUI frame if enabled
    if (config_.enableGui && guiManager_) {
        guiManager_->newFrame();
        renderGui();  // Call derived class GUI rendering
    }

    // Record command buffer - delegate to derived class
    recordRenderCommands(commandManager_->getCommandBuffer(currentFrame_), imageIndex);


    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores_[currentFrame_]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    VkCommandBuffer currentCommandBuffer = commandManager_->getCommandBuffer(currentFrame_);
    submitInfo.pCommandBuffers = &currentCommandBuffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores_[currentFrame_]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    VkResult result = vkQueueSubmit(vulkanDevice_->getGraphicsQueue(), 1, &submitInfo, inFlightFences_[currentFrame_]);
	ASSERT_VK_RESULT(result, "failed to submit draw command buffer!");

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {vulkanSwapchain_->getSwapChain()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr;

    VkResult queuePresentResult = vkQueuePresentKHR(vulkanDevice_->getPresentQueue(), &presentInfo);
    if (queuePresentResult == VK_ERROR_OUT_OF_DATE_KHR || queuePresentResult == VK_SUBOPTIMAL_KHR || framebufferResized_) {
        framebufferResized_ = false;
        recreateSwapChain();
    } else if (queuePresentResult != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame_ = (currentFrame_ + 1) % config_.maxFramesInFlight;
}

void VulkanEngine::recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window_, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window_, &width, &height);
        glfwWaitEvents();
    }
    vkDeviceWaitIdle(vulkanDevice_->getLogicalDevice());

    vulkanSwapchain_->recreate(window_);
}

void VulkanEngine::cleanup() {
    if (vulkanDevice_ && vulkanDevice_->getLogicalDevice()) {
        vkDeviceWaitIdle(vulkanDevice_->getLogicalDevice());
    }

    onCleanup();

    // Cleanup GUI
    if (guiManager_) {
        guiManager_.reset();
    }

    for (size_t i = 0; i < config_.maxFramesInFlight; i++) {
        if (vulkanDevice_ && vulkanDevice_->getLogicalDevice()) {
            if (renderFinishedSemaphores_[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(vulkanDevice_->getLogicalDevice(), renderFinishedSemaphores_[i], nullptr);
                renderFinishedSemaphores_[i] = VK_NULL_HANDLE;
            }
            if (imageAvailableSemaphores_[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(vulkanDevice_->getLogicalDevice(), imageAvailableSemaphores_[i], nullptr);
                imageAvailableSemaphores_[i] = VK_NULL_HANDLE;
            }
            if (inFlightFences_[i] != VK_NULL_HANDLE) {
                vkDestroyFence(vulkanDevice_->getLogicalDevice(), inFlightFences_[i], nullptr);
                inFlightFences_[i] = VK_NULL_HANDLE;
            }
        }
    }

    if (vulkanSwapchain_) {
        vulkanSwapchain_.reset();
    }

    if (surface_ != VK_NULL_HANDLE && vulkanInstance_) {
        vkDestroySurfaceKHR(vulkanInstance_->getInstance(), surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}

void VulkanEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
    app->framebufferResized_ = true;
}

ICommandBuffer& VulkanEngine::acquireCommandBuffer() {
	currentCommandBuffer_ = new CommandBuffer(this);
	return *currentCommandBuffer_;
}

void VulkanEngine::wait(SubmitHandle handle) {
    commandManager_->wait(handle);
}

Holder<ShaderModuleHandle> VulkanEngine::createShaderModule(const char* filename) {
	VkDevice vkDevice = vulkanDevice_.get()->getLogicalDevice();
    Shader shader = Shader(&vkDevice, filename);
#ifndef _DEBUG
    shader.createShaderModuleFromFile(vkDevice_, filename);
#else
    shader.createShaderModuleFromSPIRV(filename);
#endif
    ShaderModuleHandle handle = shaderModulesPool_.create(std::move(shader));
    return { this, handle };
}

Holder<RenderPipelineHandle> VulkanEngine::createRenderPipeline(const PipelineDesc& desc, Result* outResult) {
    
	VulkanGraphicsPipelineV2 graphicsPipeline = VulkanGraphicsPipelineV2();
    if (graphicsPipeline.getLastDescriptorSetLayout() != descriptorManager_.get()->getDescriptorSetLayout()) {
        deferredTask(std::packaged_task<void()>(
            [device = vulkanDevice_.get()->getLogicalDevice(), pipeline = graphicsPipeline.getPipeline()]() {
                vkDestroyPipeline(device, pipeline, nullptr); }));
        deferredTask(std::packaged_task<void()>(
            [device = vulkanDevice_.get()->getLogicalDevice(), layout = graphicsPipeline.getPipelineLayout()]() {
                vkDestroyPipelineLayout(device, layout, nullptr); }));
        graphicsPipeline.setPipeline(VK_NULL_HANDLE);
        graphicsPipeline.setLastDescriptorSetLayout(descriptorManager_.get()->getDescriptorSetLayout());
    }
    graphicsPipeline.createRenderPipeline(desc);
    return { this, renderPipelinesPool_.create(std::move(graphicsPipeline)) };
}

VkPipeline VulkanEngine::getVkPipeline(RenderPipelineHandle handle, uint32_t viewMask) {
	VulkanGraphicsPipelineV2* pipeline = renderPipelinesPool_.get(handle);
    VK_ASSERT(pipeline);
    /*pipeline->getVkPipeline(shaderModulesPool_, descriptorManager_.get()->getDescriptorSetLayout(),
        vulkanDevice_->getLogicalDevice(),
        pipeline->getPipelineVertexInputStateCreateInfo(),
		vulkanDevice_->getPhysicalDeviceProperties().limits);*/
	return pipeline->getPipeline();
}

Holder<BufferHandle> VulkanEngine::createBuffer(const BufferDesc& desc, const char* debugName, Result* outResult) {

    BufferManager bufferManager = BufferManager();
    bufferManager.createBuffer(desc, vulkanDevice_->getPhysicalDevice(), vulkanDevice_->getLogicalDevice(), outResult, useStaging_);
	BufferHandle handle = buffersPool_.create(std::move(bufferManager));
    if (desc.data) {
         upload(handle, desc.data, desc.size, 0);
     }
    return { this, handle };
}

Holder<SamplerHandle> VulkanEngine::createSampler(const SamplerStateDesc& desc, Result* outResult) {

    Result result;

    const VkSamplerCreateInfo info = samplerStateDescToVkSamplerCreateInfo(desc, vulkanDevice_.get()->getPhysicalDeviceProperties().limits);

    SamplerHandle handle = createSampler(desc, outResult);

    if (!VK_VERIFY(result.isOk())) {
        Result::setResult(outResult, Result(Result::Code::RuntimeError, "Cannot create Sampler"));
        return {};
    }

    Result::setResult(outResult, result);

    return { this, handle };
}

void VulkanEngine::generateMipmap(TextureHandle handle) const {
    if (handle.empty()) {
        return;
    }

    const TextureManager* tex = texturesPool_.get(handle);

    if (tex->getNumLevels() <= 1) {
        return;
    }

    VK_ASSERT(tex->getCurrentLayout() != VK_IMAGE_LAYOUT_UNDEFINED);
    const CommandBufferWrapper& wrapper = commandManager_->acquire();
    //tex->generateMipmap(wrapper.cmdBuf_);
    commandManager_->submit(wrapper);
}

Holder<TextureHandle> VulkanEngine::createTexture(const TextureDesc& desc, const char* debugName, Result* outResult) {

    TextureManager textureManager = TextureManager(vulkanDevice_.get());
    textureManager.createTexture(desc, debugName, outResult);
	TextureHandle handle = texturesPool_.create(std::move(textureManager));
    awaitingCreation_ = true;
    if (desc.data) {
       /* const uint32_t numLayers = desc.type==TextureType_Cube ? 6:1;
        upload(handle, {.dimensions   = desc.dimensions,
                        .numLayers    = numLayers,
                        .numMipLevels = desc.dataNumMipLevels},
                         desc.data);
        if (desc.generateMipmaps) this->generateMipmap(handle);*/
    }
    return { this, handle };
}

Holder<TextureHandle> VulkanEngine::createTextureView(TextureHandle texture,
    const TextureViewDesc& desc,
    const char* debugName,
    Result* outResult) {

    if (!texture) {
        VK_ASSERT(texture.valid());
        return {};
    }

    // make a copy and make it non-owning
    TextureManager tex = *texturesPool_.get(texture);
	tex.createTextureView(desc, debugName, outResult);
    TextureHandle handle = texturesPool_.create(std::move(tex));

    awaitingCreation_ = true;

    return { this, handle };
}

Holder<QueryPoolHandle> VulkanEngine::createQueryPool(uint32_t numQueries, const char* debugName, Result* outResult) {

    const VkQueryPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .flags = 0,
        .queryType = VK_QUERY_TYPE_TIMESTAMP,
        .queryCount = numQueries,
        .pipelineStatistics = 0,
    };

    VkQueryPool queryPool = VK_NULL_HANDLE;
    ASSERT_VK_RESULT(vkCreateQueryPool(vulkanDevice_.get()->getLogicalDevice(), &createInfo, 0, &queryPool), "Creating QueryPool");

    if (!queryPool) {
        Result::setResult(outResult, Result(Result::Code::RuntimeError, "Cannot create QueryPool"));
        return {};
    }

    if (debugName && *debugName) {
        VK_ASSERT(setDebugObjectName(vulkanDevice_.get()->getLogicalDevice(), VK_OBJECT_TYPE_QUERY_POOL, (uint64_t)queryPool, debugName));
    }

    QueryPoolHandle handle = queriesPool_.create(std::move(queryPool));

    return { this, handle };
}

void VulkanEngine::flushMappedMemory(BufferHandle handle, size_t offset, size_t size) const {
    const BufferManager* buf = buffersPool_.get(handle);

    VK_ASSERT(buf);

    buf->flushMappedMemory(*this, offset, size);
}

void VulkanEngine::bindDefaultDescriptorSets(VkCommandBuffer cmdBuf, VkPipelineBindPoint bindPoint, VkPipelineLayout layout) const {

    VkDescriptorSet dset = descriptorManager_.get()->getDescriptorSet();
    const VkDescriptorSet dsets[4] = { dset, dset, dset, dset };
    vkCmdBindDescriptorSets(cmdBuf, bindPoint, layout, 0, (uint32_t)VK_UTILS_GET_ARRAY_SIZE(dsets), dsets, 0, nullptr);
}

void VulkanEngine::checkAndUpdateDescriptorSets() {
    if (!awaitingCreation_) {
        // nothing to update here
        return;
    }
	descriptorManager_.get()->updateDescriptorSets(commandManager_.get());
	awaitingCreation_ = false;
}

Result VulkanEngine::upload(BufferHandle handle, const void* data, size_t size, size_t offset)
{
    BufferManager* buf = buffersPool_.get(handle);
    if (!VK_VERIFY(offset + size <= buf->getBufferSize())) {
        return Result(Result::Code ::ArgumentOutOfRange, "Out of range");
    }
    stagingDevice_->bufferSubData(*buf, offset, size, data);
    return Result();
}

Result VulkanEngine::download(BufferHandle handle, void* data, size_t size, size_t offset) {

    if (!VK_VERIFY(data)) {
        return Result(Result::Code::ArgumentOutOfRange);
    }

    VK_ASSERT_MSG(size, "Data size should be non-zero");

    BufferManager* buf = buffersPool_.get(handle);

    if (!VK_VERIFY(buf)) {
        return Result(Result::Code::ArgumentOutOfRange);
    }

    if (!VK_VERIFY(offset + size <= buf->getBufferSize())) {
        return Result(Result::Code::ArgumentOutOfRange, "Out of range");
    }

    buf->getBufferSubData(*this, offset, size, data);

    return Result();
}

SubmitHandle VulkanEngine::submit(ICommandBuffer& commandBuffer, TextureHandle present) {

    CommandBuffer* vkCmdBuffer = static_cast<CommandBuffer*>(&commandBuffer);

    VK_ASSERT(vkCmdBuffer);
    VK_ASSERT(vkCmdBuffer->eng_);
    VK_ASSERT(vkCmdBuffer->wrapper_);

#if defined(LVK_WITH_TRACY_GPU)
    TracyVkCollect(pimpl_->tracyVkCtx_, vkCmdBuffer->wrapper_->cmdBuf_);
#endif // LVK_WITH_TRACY_GPU

    if (present) {
        const TextureManager& tex = *texturesPool_.get(present);

        VK_ASSERT(tex.getIsSwapchainImage());

        tex.transitionLayout(vkCmdBuffer->wrapper_->cmdBuf_,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS });
    }

    const bool shouldPresent = hasSwapchain() && present;

    if (shouldPresent) {
        // if we a presenting a swapchain image, signal our timeline semaphore
        const uint64_t signalValue = vulkanSwapchain_->getCurrentFrameIndex() + vulkanSwapchain_->getNumSwapchainImages();
        // we wait for this value next time we want to acquire this swapchain image
        vulkanSwapchain_->setTimelineWaitValue(signalValue, vulkanSwapchain_->getCurrentFrameIndex());
        commandManager_->signalSemaphore(timelineSemaphore_, signalValue);
    }

    vkCmdBuffer->lastSubmitHandle_ = commandManager_->submit(*vkCmdBuffer->wrapper_);

    if (shouldPresent) {
        vulkanSwapchain_->present(commandManager_->acquireLastSubmitSemaphore());
    }

    processDeferredTasks();

    SubmitHandle handle = vkCmdBuffer->lastSubmitHandle_;

    // reset
    currentCommandBuffer_ = {};

    return handle;
}

uint8_t* VulkanEngine::getMappedPtr(BufferHandle handle) const {
    const BufferManager* buf = buffersPool_.get(handle);

    VK_ASSERT(buf);

    return buf->isMapped() ? buf->getMappedPtr() : nullptr;
}

uint64_t VulkanEngine::gpuAddress(BufferHandle handle, size_t offset) const {
    VK_ASSERT_MSG((offset & 7) == 0, "Buffer offset must be 8 bytes aligned as per GLSL_EXT_buffer_reference spec.");

    const BufferManager* buf = buffersPool_.get(handle);

    VK_ASSERT(buf && buf->getBufferDeviceAddress());

    return buf ? (uint64_t)buf->getBufferDeviceAddress() + offset : 0u;
}

Result VulkanEngine::upload(TextureHandle handle,
    const TextureRangeDesc& range,
    const void* data,
    uint32_t bufferRowLength) {

    if (VK_VERIFY(data)) {
        return Result(Result::Code::ArgumentOutOfRange);
    }

    TextureManager* texture = texturesPool_.get(handle);

    if (!VK_VERIFY(texture)) {
        return Result(Result::Code::ArgumentOutOfRange);
    }

    const Result result = validateRange(texture->getExtent(), texture->getNumLevels(), range);

    if (!VK_VERIFY(result.isOk())) {
        return Result(Result::Code::ArgumentOutOfRange);
    }

    const uint32_t numLayers = std::max(range.numLayers, 1u);

    VkFormat vkFormat = texture->getImageFormat();
    const VkRect2D imageRegion = {
        .offset = {.x = range.offset.x, .y = range.offset.y},
        .extent = {.width = range.dimensions.width, .height = range.dimensions.height},
    };
    stagingDevice_->imageData2D(
        *texture, 
        imageRegion, 
        range.mipLevel, 
        range.numMipLevels, 
        range.layer, 
        range.numLayers, 
        vkFormat, 
        data
    );
    return Result();
}

void VulkanEngine::deferredTask(std::packaged_task<void()>&& task, SubmitHandle handle) {
    if (handle.empty()) {
        handle = commandManager_->getNextSubmitHandle();
    }
    deferredTasks_.emplace_back(std::move(task), handle);
}

void VulkanEngine::waitDeferredTasks() {
    for (auto& task : deferredTasks_) {
        commandManager_->wait(task.handle_);
        task.task_();
    }
    deferredTasks_.clear();
}

void VulkanEngine::processDeferredTasks() {
    std::vector<DeferredTask>::iterator it = deferredTasks_.begin();

    while (it != deferredTasks_.end() && commandManager_->isReady(it->handle_)) {
        (it++)->task_();
    }

    deferredTasks_.erase(deferredTasks_.begin(), it);
}

bool VulkanEngine::hasSwapchain() const noexcept {
    return  vulkanSwapchain_ != nullptr;
}

void VulkanEngine::destroy(RenderPipelineHandle handle) {
    VulkanGraphicsPipelineV2* pipeline = renderPipelinesPool_.get(handle);

    if (!pipeline) {
        return;
    }

    free(pipeline->specConstantDataStorage_);

    deferredTask(
        std::packaged_task<void()>([device = vulkanDevice_.get()->getLogicalDevice(), pipeline = pipeline->getPipeline()]() { vkDestroyPipeline(device, pipeline, nullptr); }));
    deferredTask(std::packaged_task<void()>(
        [device = vulkanDevice_.get()->getLogicalDevice(), layout = pipeline->getPipelineLayout()]() { vkDestroyPipelineLayout(device, layout, nullptr); }));

    renderPipelinesPool_.destroy(handle);
}

void VulkanEngine::destroy(ShaderModuleHandle handle) {
    const Shader* state = shaderModulesPool_.get(handle);

    if (!state) {
        return;
    }

    if (state->getShaderModule() != VK_NULL_HANDLE) {

        vkDestroyShaderModule(vulkanDevice_.get()->getLogicalDevice(), state->getShaderModule(), nullptr);
    }

    shaderModulesPool_.destroy(handle);
}

void VulkanEngine::destroy(SamplerHandle handle) {

    VkSampler sampler = *samplersPool_.get(handle);

    samplersPool_.destroy(handle);

    deferredTask(std::packaged_task<void()>([device = vulkanDevice_.get()->getLogicalDevice(), sampler = sampler]() { vkDestroySampler(device, sampler, nullptr); }));
}

void VulkanEngine::destroy(BufferHandle handle) {

    SCOPE_EXIT{
      buffersPool_.destroy(handle);
    };

    BufferManager* buf = buffersPool_.get(handle);

    if (!buf) {
        return;
    }

    if (buf->mappedPtr_) {
        vkUnmapMemory(vulkanDevice_.get()->getLogicalDevice(), buf->getVkMemory());
    }
    deferredTask(std::packaged_task<void()>([device = vulkanDevice_.get()->getLogicalDevice(), buffer = buf->vkBuffer_, memory = buf->getVkMemory()]() {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
        }));

}

void VulkanEngine::destroy(TextureHandle handle) {

    SCOPE_EXIT{
      texturesPool_.destroy(handle);
      awaitingCreation_ = true; // make the validation layers happy
    };

    TextureManager* tex = texturesPool_.get(handle);

    if (!tex) {
        return;
    }

    deferredTask(std::packaged_task<void()>(
        [device = vulkanDevice_.get()->getLogicalDevice(), imageView = tex->getVkImageView()]() {
            vkDestroyImageView(device, imageView, nullptr);
        }));
    if (tex->getVkImageViewStorage()) {
        deferredTask(std::packaged_task<void()>(
            [device = vulkanDevice_.get()->getLogicalDevice(), imageView = tex->getVkImageViewStorage()]() { vkDestroyImageView(device, imageView, nullptr); }));
    }

    for (size_t i = 0; i != VK_MAX_MIP_LEVELS; i++) {
        for (size_t j = 0; j != VK_UTILS_GET_ARRAY_SIZE(tex->imageViewForFramebuffer_[0]); j++) {
            VkImageView v = tex->imageViewForFramebuffer_[i][j];
            if (v != VK_NULL_HANDLE) {
                deferredTask(
                    std::packaged_task<void()>([device = vulkanDevice_.get()->getLogicalDevice(), imageView = v]() { vkDestroyImageView(device, imageView, nullptr); }));
            }
        }
    }

    if (!tex->getIsOwningVkImage()) {
        return;
    }

    if (tex->mappedPtr_) {
        vkUnmapMemory(vulkanDevice_.get()->getLogicalDevice(), tex->getVkMemory());
    }
    deferredTask(std::packaged_task<void()>(
        [device = vulkanDevice_.get()->getLogicalDevice(),
        image = tex->getVkImage(),
        memory0 = tex->getVkMemory()]() {
            vkDestroyImage(device, image, nullptr);
            if (memory0 != VK_NULL_HANDLE) {
                vkFreeMemory(device, memory0, nullptr);
            }
        }
    ));
}

void VulkanEngine::destroy(QueryPoolHandle handle) {
    VkQueryPool pool = *queriesPool_.get(handle);

    queriesPool_.destroy(handle);

    deferredTask(std::packaged_task<void()>([device = vulkanDevice_.get()->getLogicalDevice(),  pool = pool]() { vkDestroyQueryPool(device, pool, nullptr); }));
}

//void VulkanEngine::destroy(AccelStructHandle handle) {
//    AccelerationStructure* accelStruct = accelStructuresPool_.get(handle);
//
//    SCOPE_EXIT{
//      accelStructuresPool_.destroy(handle);
//    };
//
//    deferredTask(std::packaged_task<void()>(
//        [device = vulkanDevice_.get()->getLogicalDevice(), as = accelStruct->vkHandle]() { vkDestroyAccelerationStructureKHR(device, as, nullptr); }));
//}

void VulkanEngine::destroy(Framebuffer& fb) {
    auto destroyFbTexture = [this](TextureHandle& handle) {
        {
            if (handle.empty())
                return;
            TextureManager* tex = texturesPool_.get(handle);
            if (!tex || !tex->getIsOwningVkImage())
                return;
            destroy(handle);
            handle = {};
        }
        };

    for (Framebuffer::AttachmentDesc& a : fb.color) {
        destroyFbTexture(a.texture);
        destroyFbTexture(a.resolveTexture);
    }
    destroyFbTexture(fb.depthStencil.texture);
    destroyFbTexture(fb.depthStencil.resolveTexture);
}













