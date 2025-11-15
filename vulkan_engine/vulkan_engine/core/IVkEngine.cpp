#include "../core/IVkEngine.h"


//void destroy(IVkEngine* eng, ComputePipelineHandle handle) {
//    if (eng) {
//        eng->destroy(handle);
//    }
//}

void destroy(IVkEngine* eng, RenderPipelineHandle handle) {
    if (eng) {
        eng->destroy(handle);
    }
}

//void destroy(IVkEngine* eng, RayTracingPipelineHandle handle) {
//    if (eng) {
//        eng->destroy(handle);
//    }
//}

void destroy(IVkEngine* eng, ShaderModuleHandle handle) {
    if (eng) {
        eng->destroy(handle);
    }
}

void destroy(IVkEngine* eng, SamplerHandle handle) {
    if (eng) {
        eng->destroy(handle);
    }
}

void destroy(IVkEngine* eng, BufferHandle handle) {
    if (eng) {
        eng->destroy(handle);
    }
}

void destroy(IVkEngine* eng, TextureHandle handle) {
    if (eng) {
        eng->destroy(handle);
    }
}

void destroy(IVkEngine* eng, QueryPoolHandle handle) {
    if (eng) {
        eng->destroy(handle);
    }
}

//void destroy(IVkEngine* eng, AccelStructHandle handle) {
//    if (eng) {
//        eng->destroy(handle);
//    }
//}