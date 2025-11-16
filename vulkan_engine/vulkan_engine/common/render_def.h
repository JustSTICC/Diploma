#pragma once
#include "../common/render_e.h"
#include "../common/VertexInput.h"
#include "../common/ObjectManager.h"
#include <future>


#define VK_MAX_COLOR_ATTACHMENTS 8
#define VK_SPECIALIZATION_CONSTANTS_MAX 16
#define VK_VERTEX_ATTRIBUTES_MAX 16
#define VK_VERTEX_BUFFER_MAX 16
#define VK_MAX_MIP_LEVELS 16

using ComputePipelineHandle =      Handle<struct ComputePipeline>;
using RenderPipelineHandle =       Handle<struct RenderPipeline>;
using RayTracingPipelineHandle =   Handle<struct RayTracingPipeline>;
using ShaderModuleHandle =         Handle<struct ShaderModule>;
using SamplerHandle =              Handle<struct Sampler>;
using BufferHandle =               Handle<struct Buffer>;
using TextureHandle =              Handle<struct Texture>;
using QueryPoolHandle =            Handle<struct QueryPool>;
using AccelStructHandle =          Handle<struct AccelerationStructure>;

struct SpecializationConstantEntry {
    uint32_t constantId = 0;
    uint32_t offset = 0; // offset within SpecializationConstantDesc::data
    size_t size = 0;
};

struct SpecializationConstantDesc {
    SpecializationConstantEntry entries[VK_SPECIALIZATION_CONSTANTS_MAX] = {};
    const void* data = nullptr;
    size_t dataSize = 0;
    uint32_t getNumSpecializationConstants() const {
        uint32_t n = 0;
        while (n < VK_SPECIALIZATION_CONSTANTS_MAX && this->entries[n].size) {
            n++;
        }
        return n;
    }
};

struct ColorAttachment {
    VkFormat format = VK_FORMAT_UNDEFINED;
    bool blendEnabled = false;
    VkBlendOp rgbBlendOp = VK_BLEND_OP_ADD;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcRGBBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstRGBBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
};

struct StencilState {
    VkStencilOp stencilFailureOp = VK_STENCIL_OP_KEEP;
    VkStencilOp depthFailureOp = VK_STENCIL_OP_KEEP;
    VkStencilOp depthStencilPassOp = VK_STENCIL_OP_KEEP;
    VkCompareOp stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    uint32_t readMask = (uint32_t)~0;
    uint32_t writeMask = (uint32_t)~0;
};


struct Viewport {
    float x = 0.0f;
    float y = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
    VkFormatProperties props;
};

struct RenderDesc final {
    struct AttachmentDesc final {
        LoadOp_e loadOp = LoadOp_Invalid;
        StoreOp_e storeOp = StoreOp_Store;
        ResolveMode_e resolveMode = ResolveMode_Average;
        uint8_t layer = 0;
        uint8_t level = 0;
        VkClearColorValue clearColor = { .float32 = {0.0f, 0.0f, 0.0f, 0.0f} };
        float clearDepth = 1.0f;
        uint32_t clearStencil = 0;
    };

    AttachmentDesc color[VK_MAX_COLOR_ATTACHMENTS] = {};
    AttachmentDesc depth = { .loadOp = LoadOp_DontCare, .storeOp = StoreOp_DontCare };
    AttachmentDesc stencil = { .loadOp = LoadOp_Invalid, .storeOp = StoreOp_DontCare };

    uint32_t layerCount = 1;
    uint32_t viewMask = 0;

    uint32_t getNumColorAttachments() const {
        uint32_t n = 0;
        while (n < VK_MAX_COLOR_ATTACHMENTS && color[n].loadOp != LoadOp_Invalid) {
            n++;
        }
        return n;
    }
};

struct PipelineDesc {
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VertexInput vertexInput;
    ShaderModuleHandle smVert;
    ShaderModuleHandle smTesc;
    ShaderModuleHandle smTese;
    ShaderModuleHandle smGeom;
    ShaderModuleHandle smTask;
    ShaderModuleHandle smFrag;
    SpecializationConstantDesc specInfo = {};
    const char* entryPointVert = "main";
    const char* entryPointTesc = "main";
    const char* entryPointTese = "main";
    const char* entryPointGeom = "main";
    const char* entryPointTask = "main";
    const char* entryPointFrag = "main";
    ColorAttachment color[VK_MAX_COLOR_ATTACHMENTS] = {};
    uint32_t getNumColorAttachments() const {
        uint32_t n = 0;
        while (n < VK_MAX_COLOR_ATTACHMENTS &&
            color[n].format != VK_FORMAT_UNDEFINED) n++;
        return n;
    }
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkFormat stencilFormat = VK_FORMAT_UNDEFINED;
    VkCullModeFlags cullMode = VK_CULL_MODE_NONE;
    VkFrontFace frontFaceWinding = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    StencilState backFaceStencil = {};
    StencilState frontFaceStencil = {};
    uint32_t samplesCount = 1u;
    uint32_t patchControlPoints = 0;
    float minSampleShading = 0.0f;
};

struct ShaderModuleDesc {
    VkShaderStageFlagBits stage_ = VK_SHADER_STAGE_FRAGMENT_BIT;
    const char* data_ = nullptr;
    size_t dataSize = 0;
    const char* debugName = "";
    ShaderModuleDesc(const char* source, VkShaderStageFlagBits stage,
        const char* debugName) : stage_(stage), data_(source),
        debugName(debugName) {
    }
    ShaderModuleDesc(const void* data, size_t dataLength,
        VkShaderStageFlagBits stage, const char* debugName) :
        stage_(stage), data_(static_cast<const char*>(data)),
        dataSize(dataLength), debugName(debugName) {
    }
};

struct ComputePipelineDesc final {
    ShaderModuleHandle smComp;
    SpecializationConstantDesc specInfo = {};
    const char* entryPoint = "main";
    const char* debugName = "";
};

struct BufferDesc final {
    uint8_t usage = 0;
    StorageType_e storage = StorageType_HostVisible;
    size_t size = 0;
    const void* data = nullptr;
    const char* debugName = "";
};

struct CommandBufferWrapper {
    VkCommandBuffer cmdBuf_ = VK_NULL_HANDLE;
    VkCommandBuffer cmdBufAllocated_ = VK_NULL_HANDLE;
    SubmitHandle handle_ = {};
    VkFence fence_ = VK_NULL_HANDLE;
    VkSemaphore semaphore_ = VK_NULL_HANDLE;
    bool isEncoding_ = false;
};

struct Framebuffer final {
    struct AttachmentDesc {
        TextureHandle texture;
        TextureHandle resolveTexture;
    };

    AttachmentDesc color[VK_MAX_COLOR_ATTACHMENTS] = {};
    AttachmentDesc depthStencil;

    const char* debugName = "";

    uint32_t getNumColorAttachments() const {
        uint32_t n = 0;
        while (n < VK_MAX_COLOR_ATTACHMENTS && color[n].texture) {
            n++;
        }
        return n;
    }
};

struct SamplerStateDesc {
    SamplerFilter_e minFilter = SamplerFilter_Linear;
    SamplerFilter_e magFilter = SamplerFilter_Linear;
    SamplerMip_e mipMap = SamplerMip_Disabled;
    SamplerWrap_e wrapU = SamplerWrap_Repeat;
    SamplerWrap_e wrapV = SamplerWrap_Repeat;
    SamplerWrap_e wrapW = SamplerWrap_Repeat;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    uint8_t mipLodMin = 0;
    uint8_t mipLodMax = 15;
    uint8_t maxAnisotropic = 1;
    bool depthCompareEnabled = false;
    const char* debugName = "";
};

struct Dependencies {
  enum { LVK_MAX_SUBMIT_DEPENDENCIES = 4 };
  TextureHandle textures[LVK_MAX_SUBMIT_DEPENDENCIES] = {};
  BufferHandle buffers[LVK_MAX_SUBMIT_DEPENDENCIES] = {};
};

struct Dimensions {
    uint32_t width = 1;
    uint32_t height = 1;
    uint32_t depth = 1;
    inline Dimensions divide1D(uint32_t v) const {
        return { .width = width / v, .height = height, .depth = depth };
    }
    inline Dimensions divide2D(uint32_t v) const {
        return { .width = width / v, .height = height / v, .depth = depth };
    }
    inline Dimensions divide3D(uint32_t v) const {
        return { .width = width / v, .height = height / v, .depth = depth / v };
    }
    inline bool operator==(const Dimensions& other) const {
        return width == other.width && height == other.height && depth == other.depth;
    }
};

struct TextureDesc {
    TextureType_e type = TextureType_2D;
    Format_e format = Format_Invalid;

    Dimensions dimensions = { 1, 1, 1 };
    uint32_t numLayers = 1;
    uint32_t numSamples = 1;
    uint8_t usage = TextureUsageBits_Sampled;
    uint32_t numMipLevels = 1;
    StorageType_e storage = StorageType_Device;
    VkComponentMapping swizzle = {};
    const void* data = nullptr;
	const char* dataPath = nullptr; // path to image file to load data from
    uint32_t dataNumMipLevels = 1; // how many mip-levels we want to upload
    bool generateMipmaps = false; // generate mip-levels immediately, valid only with non-null data
    const char* debugName = "";
    bool identity() const {
        return swizzle.r == VK_COMPONENT_SWIZZLE_IDENTITY && 
            swizzle.g == VK_COMPONENT_SWIZZLE_IDENTITY &&
            swizzle.b == VK_COMPONENT_SWIZZLE_IDENTITY &&
            swizzle.a == VK_COMPONENT_SWIZZLE_IDENTITY;
    }
};

struct TextureViewDesc {
    TextureType_e type = TextureType_2D;
    uint32_t layer = 0;
    uint32_t numLayers = 1;
    uint32_t mipLevel = 0;
    uint32_t numMipLevels = 1;
    VkComponentMapping swizzle = {};
    bool identity() const {
        return swizzle.r == VK_COMPONENT_SWIZZLE_IDENTITY &&
            swizzle.g == VK_COMPONENT_SWIZZLE_IDENTITY &&
            swizzle.b == VK_COMPONENT_SWIZZLE_IDENTITY &&
            swizzle.a == VK_COMPONENT_SWIZZLE_IDENTITY;
    }
};

struct TextureLayers {
    uint32_t mipLevel = 0;
    uint32_t layer = 0;
    uint32_t numLayers = 1;
};

struct TextureFormatProperties {
    const Format_e format = Format_Invalid;
    const uint8_t bytesPerBlock : 5 = 1;
    const uint8_t blockWidth : 3 = 1;
    const uint8_t blockHeight : 3 = 1;
    const uint8_t minBlocksX : 2 = 1;
    const uint8_t minBlocksY : 2 = 1;
    const bool depth : 1 = false;
    const bool stencil : 1 = false;
    const bool compressed : 1 = false;
    const uint8_t numPlanes : 2 = 1;
};

struct TextureRangeDesc {
    VkOffset3D offset = {};
    Dimensions dimensions = { 1, 1, 1 };
    uint32_t layer = 0;
    uint32_t numLayers = 1;
    uint32_t mipLevel = 0;
    uint32_t numMipLevels = 1;
};

#define PROPS(fmt, bpb, ...) \
  TextureFormatProperties { .format = Format_##fmt, .bytesPerBlock = bpb, ##__VA_ARGS__ }

struct ScissorRect {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct DepthState {
    VkCompareOp compareOp = VK_COMPARE_OP_ALWAYS;
    bool isDepthWriteEnabled = false;
};

struct DeferredTask {
    DeferredTask(std::packaged_task<void()>&& task, SubmitHandle handle) : task_(std::move(task)), handle_(handle) {}

    // Make DeferredTask move-only (std::packaged_task is non-copyable)
    DeferredTask(const DeferredTask&) = delete;
    DeferredTask& operator=(const DeferredTask&) = delete;
    DeferredTask(DeferredTask&&) noexcept = default;
    DeferredTask& operator=(DeferredTask&&) noexcept = default;

    std::packaged_task<void()> task_;
    SubmitHandle handle_;
};