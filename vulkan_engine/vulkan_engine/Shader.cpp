#include "Shader.h"
#include <fstream>
#include <iostream>
#include "FilePaths.h"
#include "spirv/unified1/spirv.h"
#include "utils/ScopeExit.h"
#include "validation/VulkanValidator.h"


Shader::Shader(const VkDevice* device, const char* filename) : 
    vkDevice_(device), 
    filename_(filename) {}

Shader::~Shader() {
	cleanup();
}

//void Shader::initialize(const VkDevice& device, const char* filename) {
//    vkDevice_ = device;
//#ifndef _DEBUG
//	shaderModule_ = createShaderModuleFromFile(filename);
//#else
//    shaderModule_ = createShaderModuleFromSPIRV(filename);
//#endif
//
//	//createShaderModule(device.getLogicalDevice());
//}

void Shader::cleanup() {
    vkDestroyShaderModule(*vkDevice_, shaderModule_, nullptr);
}


//std::vector<VkShaderEXT> Shader::createShaderModule(VkDevice device) {
//    VkShaderCreateFlagsEXT flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
//    VkShaderStageFlags nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
//    std::string vertexSrc, fragmentSrc;
//
//    if (readFile(VERTEX_SHADER_PATH, vertexSrc)) {
//        throw std::runtime_error("Failed to read vertex shader file");
//    }
//    VkShaderCodeTypeEXT codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
//    const char* pName = "main";
//    VkShaderCreateInfoEXT vertexInfo{};
//    vertexInfo.flags = flags;
//    vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//    vertexInfo.nextStage = nextStage;
//    vertexInfo.codeType = codeType;
//    vertexInfo.codeSize = vertexSrc.size();
//    vertexInfo.pCode = vertexSrc.data();
//    vertexInfo.pName = pName;
//
//    if (readFile(FRAGMENT_SHADER_PATH, fragmentSrc)) {
//		throw std::runtime_error("Failed to read fragment shader file");
//    }
//
//    VkShaderCreateInfoEXT fragmentInfo{};
//    fragmentInfo.flags = flags;
//    fragmentInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//    fragmentInfo.codeType = codeType;
//    fragmentInfo.codeSize = fragmentSrc.size();
//    fragmentInfo.pCode = fragmentSrc.data();
//    fragmentInfo.pName = pName;
//
//    std::vector<VkShaderCreateInfoEXT> shaderInfo = {
//        vertexInfo,
//        fragmentInfo
//    };
//    //shaderInfo.push_back(createVertexShaderInfo());
//    //shaderInfo.push_back(createFragmentShaderInfo());
//
//    std::vector<VkShaderEXT> shaders(shaderInfo.size());
//    auto result = vkCreateShadersEXT(device, shaderInfo.size(), shaderInfo.data(), nullptr, shaders.data());
// 
//    if (result == VK_SUCCESS) {
//        std::cout << "Shader modules created successfully - " << result << std::endl;
//
//        return shaders;
//    }
//    else {
//        std::cerr << "Failed to create shader modules - " << result << std::endl;
//        throw std::runtime_error("failed to create shader modules!");
//    }
//    return shaders;
//}

VkShaderModule Shader::createShaderModuleFromSPIRV(const char* pFilename) {
    int codeSize = 0;
    char* pShaderCode = readBinaryFile(pFilename, codeSize);
    assert(pShaderCode);

    VkShaderModuleCreateInfo shaderCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = (size_t)codeSize,
        .pCode = (const uint32_t*)pShaderCode
    };

    VkShaderModule shaderModule;
    VkResult res = vkCreateShaderModule(*vkDevice_, &shaderCreateInfo, NULL, &shaderModule);
    ASSERT_VK_RESULT(res, "vkCreateShaderModule\n");
    printf("Created shader from binary %s\n", pFilename);

    free(pShaderCode);

    return shaderModule;
}
//
//
//VkShaderCreateInfoEXT Shader::createVertexShaderInfo() {
//    VkShaderCreateFlagsEXT flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
//    VkShaderStageFlags nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
//    std::string vertexSrc, fragmentSrc;
//
//    if (readFile(VERTEX_SHADER_PATH, vertexSrc)) {
//		throw std::runtime_error("Failed to read vertex shader file");
//    }
//    VkShaderCodeTypeEXT codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
//    const char* pName = "main";
//    VkShaderCreateInfoEXT vertexInfo{};
//    vertexInfo.flags = flags;
//    vertexInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//    vertexInfo.nextStage = nextStage;
//    vertexInfo.codeType = codeType;
//    vertexInfo.codeSize = vertexSrc.size();
//    vertexInfo.pCode = vertexSrc.data();
//    vertexInfo.pName = pName;
//    return vertexInfo;
//}

//VkShaderCreateInfoEXT Shader::createFragmentShaderInfo() {
//
//    VkShaderCreateFlagsEXT flags = VK_SHADER_CREATE_LINK_STAGE_BIT_EXT;
//    VkShaderStageFlags nextStage = VK_SHADER_STAGE_FRAGMENT_BIT;
//
//    readFile(FRAGMENT_SHADER_PATH, fragmentSrc);
//
//    VkShaderCodeTypeEXT codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
//    const char* pName = "main";
//    VkShaderCreateInfoEXT fragmentInfo{};
//    fragmentInfo.flags = flags;
//    fragmentInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//    fragmentInfo.codeType = codeType;
//    fragmentInfo.codeSize = fragmentSrc.size();
//    fragmentInfo.pCode = fragmentSrc.data();
//    fragmentInfo.pName = pName;
//    return fragmentInfo;
//}


#ifndef _DEBUG

bool Shader::compileShader(glslang_stage_t stage, const char* sahderCode) {
    const glslang_input_t input = {
        .language = GLSLANG_SOURCE_GLSL,
        .stage = stage,
        .client = GLSLANG_CLIENT_VULKAN,
        .client_version = GLSLANG_TARGET_VULKAN_1_3,
        .target_language = GLSLANG_TARGET_SPV,
        .target_language_version = GLSLANG_TARGET_SPV_1_6,
        .code = sahderCode,
        .default_version = 100,
        .default_profile = GLSLANG_NO_PROFILE,
        .force_default_version_and_profile = false,
        .forward_compatible = false,
        .messages = GLSLANG_MSG_DEFAULT_BIT,
        .resource = glslang_default_resource()
    };

    glslang_shader_t* shader = glslang_shader_create(&input);
    if (!glslang_shader_preprocess(shader, &input)) {
        fprintf(stderr, "GLSL preprocessing failed\n");
        fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
        fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
        displayShaderCode(input.code);
        return 0;
    }

    if (!glslang_shader_parse(shader, &input)) {
        fprintf(stderr, "GLSL parsing failed\n");
        fprintf(stderr, "\n%s", glslang_shader_get_info_log(shader));
        fprintf(stderr, "\n%s", glslang_shader_get_info_debug_log(shader));
        displayShaderCode(glslang_shader_get_preprocessed_code(shader));
        return 0;
    }
    SCOPE_EXIT{
      glslang_shader_delete(shader);
    };
    
    glslang_program_t* program = glslang_program_create();
    glslang_program_add_shader(program, shader);
    SCOPE_EXIT{
      glslang_program_delete(program);
    };
    if (!glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT)) {
        fprintf(stderr, "GLSL linking failed\n");
        fprintf(stderr, "\n%s", glslang_program_get_info_log(program));
        fprintf(stderr, "\n%s", glslang_program_get_info_debug_log(program));
        return 0;
    }
    glslang_spv_options_t options = {
    .generate_debug_info = true,
    .strip_debug_info = false,
    .disable_optimizer = false,
    .optimize_size = true,
    .disassemble = false,
    .validate = true,
    .emit_nonsemantic_shader_debug_info = false,
    .emit_nonsemantic_shader_debug_source = false,
    };
    glslang_program_SPIRV_generate_with_options(program, input.stage, &options);

    if (glslang_program_SPIRV_get_messages(program)) {
        printf("%s\n", glslang_program_SPIRV_get_messages(program));
    }

	return true;
}


void Shader::displayShaderCode(const char* text)
{
    int line = 1;
    printf("\n(%3i) ", line);
    while (text && *text++)
    {
        if (*text == '\n') {
            printf("\n(%3i) ", ++line);
        }
        else if (*text == '\r') {
            // nothing to do
        }
        else {
            printf("%c", *text);
        }
    }
    printf("\n");
}

glslang_stage_t Shader::getShaderStageFromFilename(const char* pFilename)
{
    std::string s(pFilename);
    if (s.ends_with(".vert")) {
        return GLSLANG_STAGE_VERTEX;
    }
    if (s.ends_with(".frag")) {
        return GLSLANG_STAGE_FRAGMENT;
    }
    if (s.ends_with(".geom")) {
        return GLSLANG_STAGE_GEOMETRY;
    }
    if (s.ends_with(".comp")) {
        return GLSLANG_STAGE_COMPUTE;
    }
    if (s.ends_with(".tesc")) {
        return GLSLANG_STAGE_TESSCONTROL;
    }
    if (s.ends_with(".tese")) {
        return GLSLANG_STAGE_TESSEVALUATION;
    }

    printf("Unknown shader stage in '%s'\n", pFilename);
    exit(1);

    return GLSLANG_STAGE_VERTEX;
}

glslang_stage_t Shader::getGLSLangShaderStage(VkShaderStageFlagBits stage) {
    switch (stage) {
    case VK_SHADER_STAGE_VERTEX_BIT:
        return GLSLANG_STAGE_VERTEX;
    case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:
        return GLSLANG_STAGE_TESSCONTROL;
    case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:
        return GLSLANG_STAGE_TESSEVALUATION;
    case VK_SHADER_STAGE_GEOMETRY_BIT:
        return GLSLANG_STAGE_GEOMETRY;
    case VK_SHADER_STAGE_FRAGMENT_BIT:
        return GLSLANG_STAGE_FRAGMENT;
    case VK_SHADER_STAGE_COMPUTE_BIT:
        return GLSLANG_STAGE_COMPUTE;
    default:
        return GLSLANG_STAGE_COUNT;
    }
}

void Shader::setProgram(glslang_program_t* program) {
    size_t program_size = glslang_program_SPIRV_get_size(program);
    spirv_.resize(program_size);
    glslang_program_SPIRV_get(program, spirv_.data());
}

VkShaderModule Shader::createShaderModuleFromFile(const char* pFilename) {
    std::string source;
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    if (!readFile(pFilename, source)) {
        throw std::runtime_error("Failed to read shader file");
    }
    glslang_stage_t stage = getShaderStageFromFilename(pFilename);
    glslang_initialize_process();
    bool success = compileShader(stage, source.c_str());
    const VkShaderModuleCreateInfo ci = {
   .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
   .codeSize = sizeof(spirv_),
   .pCode = (const uint32_t*)spirv_.data(),
    };
    if (vkCreateShaderModule(vkDevice_, &ci, NULL, &shaderModule) == VK_SUCCESS) {
        printf("Created shader from text file!!!");
    }
}
#endif // !_DEBUG
bool Shader::endsWith(const char* s, const char* part)
{
    const size_t sLength = strlen(s);
    const size_t partLength = strlen(part);
    return sLength < partLength ? false : strcmp(s + sLength - partLength, part) == 0;
}

char* Shader::readBinaryFile(const char* filename, int& codeSize) {
    FILE* f = nullptr;
    errno_t err = fopen_s(&f, filename, "rb");
    char buf[256];
    if (err != 0 || !f) {
        printf("Error opening '%s': %s\n", filename, strerror_s(buf, sizeof(buf), errno) == 0 ? buf : "Unknown error");
        exit(0);
    }
    if (!f) {
        printf("Error opening '%s': %s\n", filename, strerror_s(buf, sizeof(buf), errno) == 0 ? buf : "Unknown error");
        exit(0);
    }

    struct stat stat_buf;
    int error = stat(filename, &stat_buf);

    if (error) {
        printf("Error getting file stats: %s\n", strerror_s(buf, sizeof(buf), errno) == 0 ? buf : "Unknown error");
        return NULL;
    }

    codeSize = stat_buf.st_size;

    char* p = (char*)malloc(codeSize);
    assert(p);

    size_t bytes_read = fread(p, 1, codeSize, f);

    if (bytes_read != codeSize) {
        printf("Read file error file: %s\n", strerror_s(buf, sizeof(buf), errno) == 0 ? buf : "Unknown error");
        exit(0);
    }
    fclose(f);
    return p;
}

bool Shader::readFile(const char* filename, std::string& outFile) {
    std::ifstream file(filename);

    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            outFile.append(line);
            outFile.append("\n");
        }
    }
    else {
        return false;
    }
}




