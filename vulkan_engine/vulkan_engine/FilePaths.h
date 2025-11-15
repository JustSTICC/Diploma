#pragma once
#ifndef _DEBUG
#define VERTEX_SHADER_PATH "../shaders/shader.vert"
#define FRAGMENT_SHADER_PATH "../shaders/shader.frag"
#else
#define VERTEX_SHADER_PATH "../shaders/vert.spv"
#define FRAGMENT_SHADER_PATH "../shaders/frag.spv"
#endif // !_DEBUG

#define VERT_SHADER_DEST "../shaders/"
#define FRAG_SHADER_DEST "../shaders/"