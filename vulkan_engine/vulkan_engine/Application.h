#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <GLFW/glfw3.h>

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <cstdint>
#include <array>
#include <unordered_map>

#include "common/Vertex.h"
#include "common/VertexTypes.h"
#include "core/VulkanEngine.h"
#include "rendering/VulkanGraphicsPipeline.h"
#include "rendering/CommandManager.h"
#include "resources/BufferManager.h"
#include "resources/TextureManager.h"
#include "descriptors/DescriptorManager.h"
#include "ui/GuiManager.h"


struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class Application : public VulkanEngine {

private:
    std::vector<StandardVertex> vertices_;
    std::vector<uint32_t> indices_;
    VkBuffer vertexBuffer_;
    VkDeviceMemory vertexBufferMemory_;
    VkBuffer indexBuffer_;
    VkDeviceMemory indexBufferMemory_;
    std::vector<VkBuffer> uniformBuffers_;
    std::vector<VkDeviceMemory> uniformBuffersMemory_;
    std::vector<void*> uniformBuffersMapped_;
    std::vector<VkDescriptorSet> descriptorSets_;
    VkImage textureImage_;
    VkDeviceMemory textureImageMemory_;
    VkImageView textureImageView_;
    VkSampler textureSampler_;
    VkImage depthImage_;
    VkDeviceMemory depthImageMemory_;
    VkImageView depthImageView_;
    std::vector<VkFramebuffer> swapChainFramebuffers_;

    ImVec4 clear_color = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);

    // Transformation control variables
    glm::vec3 modelPosition = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 modelRotation = glm::vec3(0.0f, 0.0f, 0.0f); // Euler angles in degrees
    glm::vec3 modelScale = glm::vec3(1.0f, 1.0f, 1.0f);
    bool show_transform_window = true;

public:
    Application(const Config& config) : VulkanEngine(config) {}
protected:
    void renderGui() override;
    void initializeResources() override;
    void updateUniforms(uint32_t currentImage) override;
    void recordRenderCommands(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;
    void onCleanup() override;
	void draw() override;
private:
    void loadModel();
    void createFramebuffers();

};