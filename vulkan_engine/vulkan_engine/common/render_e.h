#pragma once
#include <cstdint>

enum LoadOp_e : uint8_t {
    LoadOp_Invalid = 0,
    LoadOp_DontCare,
    LoadOp_Load,
    LoadOp_Clear,
    LoadOp_None,
};

enum StoreOp_e : uint8_t {
    StoreOp_DontCare = 0,
    StoreOp_Store,
    StoreOp_MsaaResolve,
    StoreOp_None,
};

enum ResolveMode_e : uint8_t {
    ResolveMode_None = 0,
    ResolveMode_SampleZero,
    ResolveMode_Average,
    ResolveMode_Min,
    ResolveMode_Max,
};

enum StorageType_e {
    StorageType_Device,
    StorageType_HostVisible,
    StorageType_Memoryless
};

enum SamplerFilter_e : uint8_t { SamplerFilter_Nearest = 0, SamplerFilter_Linear };
enum SamplerMip_e : uint8_t { SamplerMip_Disabled = 0, SamplerMip_Nearest, SamplerMip_Linear };
enum SamplerWrap_e : uint8_t { SamplerWrap_Repeat = 0, SamplerWrap_Clamp, SamplerWrap_MirrorRepeat };

enum BufferUsageBits : uint8_t {
    BufferUsageBits_Index = 1 << 0,
    BufferUsageBits_Vertex = 1 << 1,
    BufferUsageBits_Uniform = 1 << 2,
    BufferUsageBits_Storage = 1 << 3,
    BufferUsageBits_Indirect = 1 << 4,
};

enum TextureType_e : uint8_t {
    TextureType_2D,
    TextureType_3D,
    TextureType_Cube,
};

enum TextureUsageBits_e : uint8_t {
    TextureUsageBits_Sampled = 1 << 0,
    TextureUsageBits_Storage = 1 << 1,
    TextureUsageBits_Attachment = 1 << 2,
};

enum IndexFormat_e : uint8_t {
    IndexFormat_UI8,
    IndexFormat_UI16,
    IndexFormat_UI32,
};

enum class VertexFormat_e {
    Invalid = 0,

    Float1,
    Float2,
    Float3,
    Float4,

    Byte1,
    Byte2,
    Byte3,
    Byte4,

    UByte1,
    UByte2,
    UByte3,
    UByte4,

    Short1,
    Short2,
    Short3,
    Short4,

    UShort1,
    UShort2,
    UShort3,
    UShort4,

    Byte2Norm,
    Byte4Norm,

    UByte2Norm,
    UByte4Norm,

    Short2Norm,
    Short4Norm,

    UShort2Norm,
    UShort4Norm,

    Int1,
    Int2,
    Int3,
    Int4,

    UInt1,
    UInt2,
    UInt3,
    UInt4,

    HalfFloat1,
    HalfFloat2,
    HalfFloat3,
    HalfFloat4,

    Int_2_10_10_10_REV,
};

enum Format_e : uint8_t {
    Format_Invalid = 0,

    Format_R_UN8,
    Format_R_UI16,
    Format_R_UI32,
    Format_R_UN16,
    Format_R_F16,
    Format_R_F32,

    Format_RG_UN8,
    Format_RG_UI16,
    Format_RG_UI32,
    Format_RG_UN16,
    Format_RG_F16,
    Format_RG_F32,

    Format_RGBA_UN8,
    Format_RGBA_UI32,
    Format_RGBA_F16,
    Format_RGBA_F32,
    Format_RGBA_SRGB8,

    Format_BGRA_UN8,
    Format_BGRA_SRGB8,

    Format_A2B10G10R10_UN,
    Format_A2R10G10B10_UN,

    Format_ETC2_RGB8,
    Format_ETC2_SRGB8,
    Format_BC7_RGBA,

    Format_Z_UN16,
    Format_Z_UN24,
    Format_Z_F32,
    Format_Z_UN24_S_UI8,
    Format_Z_F32_S_UI8,

    Format_YUV_NV12,
    Format_YUV_420p,
};

enum Bindings_e {
    kBinding_Textures = 0,
    kBinding_Samplers = 1,
    kBinding_StorageImages = 2,
    kBinding_YUVImages = 3,
    kBinding_AccelerationStructures = 4,
    kBinding_NumBindings = 5,
};
