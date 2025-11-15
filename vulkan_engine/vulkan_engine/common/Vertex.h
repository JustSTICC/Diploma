#pragma once

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

#define VK_VERTEX_ATTRIBUTES_MAX 16
#define VK_VERTEX_BUFFER_MAX 16 

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    //struct VertexAttribute final {
    //    uint32_t location = 0; // a buffer which contains this attribute stream
    //    uint32_t binding = 0;
    //    VkFormat format = VK_FORMAT_UNDEFINED;
    //    uintptr_t offset = 0; // an offset where the first element of this attribute stream starts
    //} attributes[VK_VERTEX_ATTRIBUTES_MAX];
    //struct VertexInputBinding final {
    //    uint32_t stride = 0;
    //} inputBindings[VK_VERTEX_BUFFER_MAX];

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();
    
    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
    
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const;
    };
}