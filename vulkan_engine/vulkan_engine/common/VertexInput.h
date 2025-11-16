#pragma once
#include <vulkan/vulkan.hpp>
#include <cstdint>

#define VK_VERTEX_ATTRIBUTES_MAX 16
#define VK_VERTEX_BUFFER_MAX 16

class VertexInput final {
public:
    struct VertexAttribute final {
        uint32_t location = 0; // a buffer which contains this attribute stream
        uint32_t binding = 0;
        VkFormat format = VK_FORMAT_UNDEFINED;
        uintptr_t offset = 0; // an offset where the first element of this attribute stream starts
    } attributes[VK_VERTEX_ATTRIBUTES_MAX];
    struct VertexInputBinding final {
        uint32_t stride = 0;
    } inputBindings[VK_VERTEX_BUFFER_MAX];

    uint32_t getNumAttributes() const {
        uint32_t n = 0;
        while (n < VK_VERTEX_ATTRIBUTES_MAX && attributes[n].format != VK_FORMAT_UNDEFINED) {
            n++;
        }
		return n;
    }

    uint32_t getNumInputBindings() const {
        uint32_t n = 0;
        while (n < VK_VERTEX_BUFFER_MAX && inputBindings[n].stride) {
            n++;
        }
        return n;
    }
    uint32_t getVertexSize() const;

    bool operator==(const VertexInput& other) const {
        return memcmp(this, &other, sizeof(VertexInput)) == 0;
    }
};

