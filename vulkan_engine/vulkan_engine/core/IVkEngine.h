#pragma once
#include "../core/ICommandBuffer.h"

template<typename HandleType>
class Holder;

class IVkEngine {
protected:
    IVkEngine() = default;

public:
    virtual ~IVkEngine() = default;

    virtual ICommandBuffer& acquireCommandBuffer() = 0;

    virtual SubmitHandle submit(ICommandBuffer& commandBuffer, TextureHandle present = {}) = 0;
    virtual void wait(SubmitHandle handle) = 0; // waiting on an empty handle results in vkDeviceWaitIdle()

    [[nodiscard]] virtual Holder<BufferHandle> createBuffer(const BufferDesc& desc,
        const char* debugName = nullptr,
        Result* outResult = nullptr) = 0;
    [[nodiscard]] virtual Holder<SamplerHandle> createSampler(const SamplerStateDesc& desc, Result* outResult = nullptr) = 0;
    [[nodiscard]] virtual Holder<TextureHandle> createTexture(const TextureDesc& desc,
        const char* debugName = nullptr,
        Result* outResult = nullptr) = 0;
    [[nodiscard]] virtual Holder<TextureHandle> createTextureView(TextureHandle texture,
        const TextureViewDesc& desc,
        const char* debugName = nullptr,
        Result* outResult = nullptr) = 0;
    //[[nodiscard]] virtual Holder<ComputePipelineHandle> createComputePipeline(const ComputePipelineDesc& desc,
     //   Result* outResult = nullptr) = 0;
    [[nodiscard]] virtual Holder<RenderPipelineHandle> createRenderPipeline(const PipelineDesc& desc, Result* outResult = nullptr) = 0;
    //[[nodiscard]] virtual Holder<RayTracingPipelineHandle> createRayTracingPipeline(const RayTracingPipelineDesc& desc,
    //    Result* outResult = nullptr) = 0;
    [[nodiscard]] virtual Holder<ShaderModuleHandle> createShaderModule(const char* filename) = 0;

    [[nodiscard]] virtual Holder<QueryPoolHandle> createQueryPool(uint32_t numQueries,
        const char* debugName,
        Result* outResult = nullptr) = 0;

    /*  [[nodiscard]] virtual Holder<AccelStructHandle> createAccelerationStructure(const AccelStructDesc& desc, Result* outResult = nullptr) = 0;*/

    virtual Result upload(BufferHandle handle, const void* data, size_t size, size_t offset = 0) = 0;
    virtual Result download(BufferHandle handle, void* data, size_t size, size_t offset) = 0;
    virtual uint8_t* getMappedPtr(BufferHandle handle) const = 0;
    virtual uint64_t gpuAddress(BufferHandle handle, size_t offset = 0) const = 0;
    virtual void flushMappedMemory(BufferHandle handle, size_t offset, size_t size) const = 0;

    virtual Result upload(TextureHandle handle, const TextureRangeDesc& range, const void* data, uint32_t bufferRowLength = 0) = 0;

    //virtual void destroy(ComputePipelineHandle handle) = 0;
    virtual void destroy(RenderPipelineHandle handle) = 0;
    //virtual void destroy(RayTracingPipelineHandle) = 0;
    virtual void destroy(ShaderModuleHandle handle) = 0;
    virtual void destroy(SamplerHandle handle) = 0;
    virtual void destroy(BufferHandle handle) = 0;
    virtual void destroy(TextureHandle handle) = 0;
    virtual void destroy(QueryPoolHandle handle) = 0;
   // virtual void destroy(AccelStructHandle handle) = 0;
    virtual void destroy(Framebuffer& fb) = 0;

};

//void destroy(IVkEngine* eng, ComputePipelineHandle handle);
void destroy(IVkEngine* eng, RenderPipelineHandle handle);
//void destroy(IVkEngine* eng, RayTracingPipelineHandle handle);
void destroy(IVkEngine* eng, ShaderModuleHandle handle);
void destroy(IVkEngine* eng, SamplerHandle handle);
void destroy(IVkEngine* eng, BufferHandle handle);
void destroy(IVkEngine* eng, TextureHandle handle);
void destroy(IVkEngine* eng, QueryPoolHandle handle);
//void destroy(IVkEngine* eng, AccelStructHandle handle);

template<typename HandleType>
class Holder final {
public:
    Holder() = default;
    Holder(IVkEngine* eng, HandleType handle) : eng_(eng), handle_(handle) {}
    ~Holder() {
        destroy(eng_, handle_);
    }
    Holder(const Holder&) = delete;
    Holder(Holder&& other) : eng_(other.eng_), handle_(other.handle_) {
        other.eng_ = nullptr;
        other.handle_ = HandleType{};
    }
    Holder& operator=(const Holder&) = delete;
    Holder& operator=(Holder&& other) {
        std::swap(eng_, other.eng_);
        std::swap(handle_, other.handle_);
        return *this;
    }
    Holder& operator=(std::nullptr_t) {
        this->reset();
        return *this;
    }

    inline operator HandleType() const {
        return handle_;
    }

    bool valid() const {
        return handle_.valid();
    }

    bool empty() const {
        return handle_.empty();
    }

    void reset() {
        destroy(eng_, handle_);
        eng_ = nullptr;
        handle_ = HandleType{};
    }

    HandleType release() {
        eng_ = nullptr;
        return std::exchange(handle_, HandleType{});
    }

    uint32_t gen() const {
        return handle_.gen();
    }
    uint32_t index() const {
        return handle_.index();
    }
    void* indexAsVoid() const {
        return handle_.indexAsVoid();
    }
    void* handleAsVoid() const {
        return handle_.handleAsVoid();
    }

private:
    IVkEngine* eng_ = nullptr;
    HandleType handle_ = {};
};


