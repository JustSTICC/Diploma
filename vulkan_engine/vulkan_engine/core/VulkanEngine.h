#pragma once
#include "../config.h"
#include "../core/IVkEngine.h"
#include "../CommandBuffer.h"
#include <GLFW/glfw3.h>
#include <memory> 



class VulkanInstance;
class Shader;
class VulkanDevice;
class VulkanSwapchain;
class VulkanGraphicsPipeline;
class VulkanGraphicsPipelineV2;
class CommandManager;
class CommandBuffer;
class BufferManager;
class TextureManager;
class DescriptorManager;
class GuiManager;
class StagingDevice;

class VulkanEngine : public IVkEngine{
    public:
        VulkanEngine(const Config& config);
        virtual ~VulkanEngine();

        Config config_;
        std::unique_ptr<CommandManager> commandManager_;
        std::unique_ptr<VulkanDevice> vulkanDevice_;
		std::vector<DeferredTask> deferredTasks_;
        CommandBuffer* currentCommandBuffer_;
        VkSemaphore timelineSemaphore_ = VK_NULL_HANDLE;

        ICommandBuffer& acquireCommandBuffer() override;

        SubmitHandle submit(ICommandBuffer& commandBuffer, TextureHandle present) override;
        void wait(SubmitHandle handle) override;

        void deferredTask(std::packaged_task<void()>&& task, SubmitHandle handle= SubmitHandle());
        void waitDeferredTasks();
        void processDeferredTasks();

        Holder<BufferHandle> createBuffer(const BufferDesc& desc, const char* debugName = nullptr, Result* outResult = nullptr) override;
        Holder<SamplerHandle> createSampler(const SamplerStateDesc& desc, Result* outResult) override;
        Holder<TextureHandle> createTexture(const TextureDesc& desc, const char* debugName = nullptr, Result* outResult = nullptr) override;
        Holder<TextureHandle> createTextureView(TextureHandle texture,
            const TextureViewDesc& desc,
            const char* debugName,
            Result* outResult) override;

        Holder<RenderPipelineHandle> createRenderPipeline(const PipelineDesc& desc, Result* outResult = nullptr) override;
        Holder<ShaderModuleHandle> createShaderModule(const char* filename) override;
        Holder<QueryPoolHandle> createQueryPool(uint32_t numQueries, const char* debugName, Result* outResult) override;

        VkPipeline getVkPipeline(RenderPipelineHandle handle, uint32_t viewMask);
        TextureHandle getCurrentSwapchainTexture();

        void checkAndUpdateDescriptorSets();
        void bindDefaultDescriptorSets(VkCommandBuffer cmdBuf, VkPipelineBindPoint bindPoint, VkPipelineLayout layout) const;

        Result upload(BufferHandle handle, const void* data, size_t size, size_t offset = 0) override;
        Result download(BufferHandle handle, void* data, size_t size, size_t offset) override;
        uint8_t* getMappedPtr(BufferHandle handle) const override;
        uint64_t gpuAddress(BufferHandle handle, size_t offset = 0) const override;
        void flushMappedMemory(BufferHandle handle, size_t offset, size_t size) const override;

        Result upload(TextureHandle handle, const TextureRangeDesc& range, const void* data, uint32_t bufferRowLength = 0) override;
        void generateMipmap(TextureHandle handle) const;

        void destroy(RenderPipelineHandle handle) override;
        void destroy(ShaderModuleHandle handle) override;
        void destroy(SamplerHandle handle) override;
        void destroy(BufferHandle handle) override;
        void destroy(TextureHandle handle) override;
        void destroy(QueryPoolHandle handle) override;
        void destroy(Framebuffer& fb) override;

        void run();

    protected:
        virtual void drawFrame() = 0;
        virtual void initializeResources() {}
        //virtual void updateUniforms(uint32_t currentImage) {}
        //virtual void recordRenderCommands(VkCommandBuffer commnadBuffer, uint32_t imageIndex) {}
        //virtual void renderGui() {}
        //virtual void onCleanup() {} 
        //virtual void draw() {}

        GLFWwindow* window_;
        VkSurfaceKHR surface_;

		Holder<ShaderModuleHandle> vertShader_;
		Holder<ShaderModuleHandle> fragShader_;
		Holder<RenderPipelineHandle> vulkanPipeline_;
		Holder<BufferHandle> buffer_;
		Holder<TextureHandle> texture_;

        std::unique_ptr<VulkanInstance> vulkanInstance_;
		//std::unique_ptr<Shader> vertShader_;
  //      std::unique_ptr<Shader> fragShader_;
        std::unique_ptr<VulkanSwapchain> vulkanSwapchain_;
        //std::unique_ptr<VulkanGraphicsPipeline> vulkanPipeline_;
        /*std::unique_ptr<BufferManager> bufferManager_;
        std::unique_ptr<TextureManager> textureManager_;*/
        std::unique_ptr<DescriptorManager> descriptorManager_;
        std::unique_ptr<GuiManager> guiManager_;
        std::unique_ptr<StagingDevice> stagingDevice_;

        std::vector<VkSemaphore> imageAvailableSemaphores_;
        std::vector<VkSemaphore> renderFinishedSemaphores_;
        std::vector<VkFence> inFlightFences_;
        uint32_t currentFrame_ = 0;
        bool framebufferResized_ = false;
        bool useStaging_ = true;
        bool awaitingCreation_ = false;



    private:
        bool hasSwapchain() const noexcept;

        void initWindow();
        void initVulkan();
        void mainLoop();
        //void drawFrame();
        void cleanup();
        void recreateSwapChain();
        void createSyncObjects();

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height);


public:
    Pool<ShaderModule, Shader> shaderModulesPool_;
    Pool<RenderPipeline, VulkanGraphicsPipelineV2> renderPipelinesPool_;
    Pool<Sampler, VkSampler> samplersPool_;
    Pool<Buffer, BufferManager> buffersPool_;
    Pool<Texture, TextureManager> texturesPool_;
    Pool<QueryPool, VkQueryPool> queriesPool_;
};
