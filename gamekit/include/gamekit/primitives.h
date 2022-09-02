/*
 * Primitives
 */
#pragma once

#include <cstdint>
#include <string>

namespace gamekit {

typedef int64_t microsecond_t;
typedef int64_t nanosecond_t;

enum class ResourceType {
    Unknown = 0x0,
    Data = 0x1,
    Text = 0x2,
    Bitmap = 0x3,
    VertexShader = 0x4,
    FragmentShader = 0x5
};

struct ResourceDescriptor {
    std::string name;
    const void* data{nullptr};
    size_t dataSize{0};
    ResourceType type{ResourceType::Unknown};
};

} // namespace
