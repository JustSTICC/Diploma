#pragma once
#include "VulkanCleaner.h"
#include <string>

class Shader
{
public:
		Shader(){}
		~Shader(){}
		static std::vector<char> readFile(const std::string& filename);
		static std::vector<VkShaderEXT> createShaderModule(VkDevice device);
		
private:
	static VkShaderCreateInfoEXT createVertexShaderInfo();
	static VkShaderCreateInfoEXT createFragmentShaderInfo();
};

