/*
 * Types
 */
#pragma once

#include <vulkan>

#include <gamekit/device.h>
#include <gamekit/vertex.h>
#include <gamekit/buffer.h>
#include "gamekit/sprite.h"

#include <vector>
#include <glm/glm.hpp>

namespace gamekit {

class VertexQueue {
    protected:
        static const size_t npos = (size_t) -1; // std::numeric_limits<std::size_t>::max();

    public:
        static VertexQueue make(size_t capacity);
        void create(size_t capacity);

    public:
        void begin();
        void end();
        void draw();
        void clear();
        size_t reserve(size_t numIndices=1);

    public:
        void update();

    protected:
        void set(const glm::vec4* optionalRect,
                 const glm::vec4* optionalColor,
                 const glm::vec4* optionalTexcoords,
                 const uint32_t* optionalTexmask,
                 const uint32_t* optionalFlags,
                 size_t index = npos);

    public:
        [[nodiscard]] size_t capacity() const { return capacity_; }
        [[nodiscard]] size_t count() const { return count_; }

    private:
        inline void checkIndex(size_t& index);

    protected:
        inline void setCoords(size_t index, float x, float y, float w, float h);
        inline void setCoords(size_t index, const glm::vec4* coords);
        inline void setColor(size_t index, float r, float g, float b, float a);
        inline void setColor(size_t index, const glm::vec4* color);
        inline void setTextureCoords(size_t index, float tx, float ty, float tw, float th);
        inline void setTextureCoords(size_t index, const glm::vec4* texture_coords);
        inline void setTextureMask(size_t index, uint32_t texture_mask);
        inline void setFlags(size_t index, uint32_t flags);

    protected:
        size_t capacity_{0};
        size_t reserved_{0};
        size_t count_{0};
        bool modified_{false};

    private:
        std::vector<Vertex> vertices_;
        std::vector<uint16_t> indices_;
        VertexBuffer vertexBuffer_;
        IndexBuffer indexBuffer_;
};

class QuadBatch : public VertexQueue {
    public:
        static QuadBatch make(size_t capacity);
        void create(size_t capacity);

    public:
        void push(const glm::vec4& rect);
        void push(const glm::vec4& rect,
                  const glm::vec4& color,
                  const glm::vec4& texcoords,
                  uint32_t texmask,
                  uint32_t flags);
        void push(float x, float y, float w, float h,
                  float r, float g, float b, float a,
                  float tx, float ty, float tw, float th,
                  uint32_t texmask, uint32_t flags);

    public:
        void store(size_t index, const glm::vec4& rect);
        void store(size_t index,
                   const glm::vec4& rect,
                   const glm::vec4& color,
                   const glm::vec4& texcoords,
                   uint32_t texmask,
                   uint32_t flags);

        void store(size_t index,
                   float x, float y, float w, float h,
                   float r, float g, float b, float a,
                   float tx, float ty, float tw, float th,
                   uint32_t texmask,
                   uint32_t flags);

};

class SpriteBatch : public VertexQueue {

    public:
        static SpriteBatch make(size_t capacity);
        void create(size_t capacity);

    public:
        void push(const Sprite& sprite);
        void store(size_t index, const Sprite& sprite);

    private:
        void set(const Sprite& sprite, size_t index=npos);

};

} // namespace
