/*
 * Types
 */
#pragma once

#include <vulkan>

#include <array>

#include "gamekit/primitives.h"

#include <glm/glm.hpp> // NOLINT

namespace gamekit {

///////////////////////////////////////////////////////////////////////////////
// Vertex
///////////////////////////////////////////////////////////////////////////////

class Vertex {
    private:
        static const size_t NUM_ATTRIBUTES = 5;

    public:
        glm::vec3 pos_;
        glm::vec4 color_;
        glm::vec2 texcoord_;
        uint32_t texmask_{0x0};
        uint32_t flags_{0x0};

    public:
        static Vertex make(const glm::vec3& pos, const glm::vec4& color, const glm::vec2& texcoord, uint32_t texmask, uint32_t flags) {
            Vertex v;
            v.set(pos, color, texcoord, texmask, flags);
            return v;
        }

    public:
        void set(const glm::vec3& pos, const glm::vec4& color, const glm::vec2& texcoord, uint32_t texmask, uint32_t flags) {
            setPos(pos);
            setColor(color);
            setTexcoord(texcoord);
            setTexmask(texmask);
            setFlags(flags);
        }

        void setPos(const glm::vec3& pos) {
            pos_ = pos;
        }

        void setPos(float x, float y, float z) {
            pos_.x = x;
            pos_.y = y;
            pos_.z = z;
        }

        void setColor(float r, float g, float b, float a) {
            color_.r = r;
            color_.g = g;
            color_.b = b;
            color_.a = a;
        }

        void setColor(const glm::vec4& color) {
            color_ = color;
        }

        void setTexcoord(float u, float v) {
            texcoord_.x = u;
            texcoord_.y = v;
        }

        void setTexcoord(const glm::vec2& texcoord) {
            texcoord_ = texcoord;
        }

        void setTexmask(uint32_t texmask) {
            texmask_ = texmask;
        }

        void setFlags(uint32_t flags) {
            flags_ = flags;
        }

    public:
        [[nodiscard]] glm::vec3& pos() { return pos_; }
        [[nodiscard]] const glm::vec3& pos() const { return pos_; }
        [[nodiscard]] glm::vec4& color() { return color_; }
        [[nodiscard]] const glm::vec4& color() const { return color_; }
        [[nodiscard]] glm::vec2& texcoord() { return texcoord_; }
        [[nodiscard]] const glm::vec2& texcoord() const { return texcoord_; }
        [[nodiscard]] uint32_t texmask() const { return texmask_; }
        [[nodiscard]] uint32_t flags() const { return flags_; }

    public:
        static VkVertexInputBindingDescription getBindingDescription();
        static std::array<VkVertexInputAttributeDescription, Vertex::NUM_ATTRIBUTES> getAttributeDescriptions();

};

} // namespace
