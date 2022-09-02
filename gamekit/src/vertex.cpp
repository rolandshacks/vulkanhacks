/*
 * Utilities
 */

#include <vulkan>

#include "gamekit/vertex.h"

using namespace gamekit;

VkVertexInputBindingDescription Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, Vertex::NUM_ATTRIBUTES> Vertex::getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, Vertex::NUM_ATTRIBUTES> attributeDescriptions{};

    int idx = 0;

    attributeDescriptions[idx].binding = 0;
    attributeDescriptions[idx].location = idx;
    attributeDescriptions[idx].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[idx].offset = offsetof(Vertex, pos_);
    idx++;

    attributeDescriptions[idx].binding = 0;
    attributeDescriptions[idx].location = idx;
    attributeDescriptions[idx].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[idx].offset = offsetof(Vertex, color_);
    idx++;

    attributeDescriptions[idx].binding = 0;
    attributeDescriptions[idx].location = idx;
    attributeDescriptions[idx].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[idx].offset = offsetof(Vertex, texcoord_);
    idx++;

    attributeDescriptions[idx].binding = 0;
    attributeDescriptions[idx].location = idx;
    attributeDescriptions[idx].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[idx].offset = offsetof(Vertex, texmask_);
    idx++;

    attributeDescriptions[idx].binding = 0;
    attributeDescriptions[idx].location = idx;
    attributeDescriptions[idx].format = VK_FORMAT_R32_UINT;
    attributeDescriptions[idx].offset = offsetof(Vertex, flags_);
    idx++;

    return attributeDescriptions;
}
