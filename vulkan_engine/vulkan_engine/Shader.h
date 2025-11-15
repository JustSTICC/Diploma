#pragma once
#include <glslang/Include/glslang_c_interface.h>
#include <glslang/Public/resource_limits_c.h>
#include "core/IVkEngine.h"


class Shader
{
public:
	Shader() = default;
	Shader(const VkDevice* device, const char* filename);
	~Shader();
	

	//void initialize(const VkDevice& device, const char* filename);
	void cleanup();
	//std::vector<VkShaderEXT> createShaderModule(VkDevice device);

	VkShaderModule createShaderModuleFromSPIRV(const char* pFilename);
	VkShaderModule createShaderModuleFromFile(const char* pFilename);
	VkShaderModule getShaderModule() const { return shaderModule_; }

	VkShaderModule shaderModule_ = VK_NULL_HANDLE;
	uint32_t pushConstantsSize = 0;
private:
	const VkDevice* vkDevice_ = VK_NULL_HANDLE;
	std::vector<uint32_t> spirv_;
	const char* filename_ = nullptr;
	

	bool endsWith(const char* s, const char* part);
	static bool readFile(const char* filename, std::string& outFile);
	static char* readBinaryFile(const char* filename, int& codeSize);
	glslang_stage_t getShaderStageFromFilename(const char* pFilename);
	glslang_stage_t getGLSLangShaderStage(VkShaderStageFlagBits stage);
#ifndef _DEBUG

//public:
//	//void testShaderCompilation(const char* sourceFilename, const char* destFilename);
//private:
	void displayShaderCode(const char* text);
	bool compileShader(glslang_stage_t stage, const char* sahderCode);
	void saveSPIRVBinaryFile(const char* filename, const uint8_t* code, size_t size);
	void setProgram(glslang_program_t* program);

#endif // !_DEBUG
};



