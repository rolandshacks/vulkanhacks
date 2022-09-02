/*
 * Sprite batch
 */

#include <vulkan>

#include <gamekit/device.h>
#include <gamekit/vertex.h>
#include <gamekit/texture.h>
#include <gamekit/buffer.h>
#include "gamekit/sprite_batch.h"

#include <array>
#include <stdexcept>
#include <glm/glm.hpp>

using namespace gamekit;

static const glm::vec4 DEFAULT_COLOR { 1.0f, 1.0f, 1.0f, 1.0f };
static const glm::vec4 DEFAULT_TEXTURE_COORDS { 0.0f, 0.0f, 1.0f, 1.0f };
static const uint32_t DEFAULT_TEXTURE_MASK = 0x1;
static const uint32_t DEFAULT_FLAGS = 0x0;

///////////////////////////////////////////////////////////////////////////////
// Vertex Queue
///////////////////////////////////////////////////////////////////////////////

VertexQueue VertexQueue::make(size_t capacity) {
    VertexQueue quadBatch;
    quadBatch.create(capacity);
    return quadBatch;
}

void VertexQueue::create(size_t capacity) {

    assert(capacity > 0);

    capacity_ = capacity;
    count_ = 0;
    modified_ = false;

    auto numIndices = capacity_ * 6;
    auto numVertices = capacity_ * 4;

    vertices_.resize(numVertices);
    indices_.resize(numIndices);

    auto ptr = indices_.data();
    size_t idx = 0;
    size_t ofs = 0;
    while (idx < capacity_ * 6) {

        // repeat indices: 3, 2, 0, 2, 1, 0

        *(ptr++) = static_cast<uint16_t>(ofs + 2);
        *(ptr++) = static_cast<uint16_t>(ofs + 1);
        *(ptr++) = static_cast<uint16_t>(ofs + 0);
        *(ptr++) = static_cast<uint16_t>(ofs + 0);
        *(ptr++) = static_cast<uint16_t>(ofs + 3);
        *(ptr++) = static_cast<uint16_t>(ofs + 2);

        idx += 6;
        ofs += 4;
    }

    indexBuffer_ = IndexBuffer::make(numIndices * sizeof(uint16_t));
    indexBuffer_.copy(indices_.data());

    vertexBuffer_ = VertexBuffer::make(numVertices * sizeof(Vertex));
}

void VertexQueue::begin() {
    count_ = 0;
}

void VertexQueue::end() {

}

void VertexQueue::clear() {
    count_ = 0;
    reserved_ = 0;
}

size_t VertexQueue::reserve(size_t numIndices) {

    if (count_ > 0) {
        throw std::runtime_error("cannot reserve after dynamic push to sprite batch");
    }

    if (count_ + reserved_ + numIndices > capacity_) {
        throw std::runtime_error("sprite batch overflow");
    }

    size_t index = reserved_;

    reserved_ += numIndices;

    return index;
}


void VertexQueue::update() {

    auto num = count_ + reserved_;

    if (!modified_ || 0 == num) {
        return;
    }

    modified_ = false;
    auto numVertices = num * 4;
    vertexBuffer_.copy(vertices_.data(), sizeof(Vertex) * numVertices);
}

void VertexQueue::draw() {
    update();

    auto num = count_ + reserved_;

    if (0 == num) return;

    auto device = Device::globalInstance();
    auto numIndices = num * 6;
    vertexBuffer_.bind();
    indexBuffer_.bind();
    device->drawIndexed(numIndices);
}

inline void VertexQueue::setCoords(size_t index, const glm::vec4* coords) {
    setCoords(index, coords->x, coords->y, coords->z, coords->w);
}

inline void VertexQueue::setCoords(size_t index, float x, float y, float w, float h) {

    auto x0 = x;
    auto y0 = y;
    auto x1 = x0 + w;
    auto y1 = y0 + h;
    auto z = 0.0f;

    auto ofs = index * 4;

    auto v = vertices_.data() + ofs;
    v->setPos(x0, y0, z); v++;
    v->setPos(x1, y0, z); v++;
    v->setPos(x1, y1, z); v++;
    v->setPos(x0, y1, z); v++;

}

inline void VertexQueue::setColor(size_t index, const glm::vec4* color) {
    setColor(index, color->r, color->g, color->b, color->a);
}

inline void VertexQueue::setColor(size_t index, float r, float g, float b, float a) {

    auto ofs = index * 4;
    auto v = vertices_.data() + ofs;
    v->setColor(r, g, b, a); v++;
    v->setColor(r, g, b, a); v++;
    v->setColor(r, g, b, a); v++;
    v->setColor(r, g, b, a); v++;
}

inline void VertexQueue::setTextureCoords(size_t index, const glm::vec4* texture_coords) {
    setTextureCoords(index, texture_coords->x, texture_coords->y, texture_coords->z, texture_coords->w);
}

inline void VertexQueue::setTextureCoords(size_t index, float tx, float ty, float tw, float th) {

    auto u0 = tx;
    auto v0 = ty;
    auto u1 = u0 + tw;
    auto v1 = v0 + th;

    auto ofs = index * 4;
    auto v = vertices_.data() + ofs;
    v->setTexcoord(u0, v0); v++;
    v->setTexcoord(u1, v0); v++;
    v->setTexcoord(u1, v1); v++;
    v->setTexcoord(u0, v1); v++;
}

inline void VertexQueue::setTextureMask(size_t index, uint32_t texture_mask) {
    auto ofs = index * 4;
    auto v = vertices_.data() + ofs;
    v->setTexmask(texture_mask); v++;
    v->setTexmask(texture_mask); v++;
    v->setTexmask(texture_mask); v++;
    v->setTexmask(texture_mask); v++;
}

inline void VertexQueue::setFlags(size_t index, uint32_t flags) {
    auto ofs = index * 4;
    auto v = vertices_.data() + ofs;
    v->setFlags(flags); v++;
    v->setFlags(flags); v++;
    v->setFlags(flags); v++;
    v->setFlags(flags); v++;
}

inline void VertexQueue::checkIndex(size_t& index) {
    if (index == npos) {
        if (count_ + reserved_ >= capacity_) {
            throw std::runtime_error("vertex queue overflow");
        }
        index = count_ + reserved_;
        count_++;
    } else {
        if (index >= count_ + reserved_) {
            throw std::runtime_error("vertex queue index out of bounds");
        }
    }
}

void VertexQueue::set(const glm::vec4* optionalRect,
                      const glm::vec4* optionalColor,
                      const glm::vec4* optionalTexcoords,
                      const uint32_t* optionalTexmask,
                      const uint32_t* optionalFlags,
                      size_t index) {

    checkIndex(index);

    if (nullptr != optionalRect) setCoords(index, optionalRect);
    if (nullptr != optionalColor) setColor(index,  optionalColor);
    if (nullptr != optionalTexcoords) setTextureCoords(index, optionalTexcoords);
    if (nullptr != optionalTexmask) setTextureMask(index, *optionalTexmask);
    if (nullptr != optionalFlags) setFlags(index, *optionalFlags);

    modified_ = true;
}

///////////////////////////////////////////////////////////////////////////////
// Quad Batch
///////////////////////////////////////////////////////////////////////////////

QuadBatch QuadBatch::make(size_t capacity) {
    QuadBatch quadBatch;
    quadBatch.create(capacity);
    return quadBatch;
}

void QuadBatch::create(size_t capacity) {
    return VertexQueue::create(capacity);
}

void QuadBatch::push(float x, float y, float w, float h,
                     float r, float g, float b, float a,
                     float tx, float ty, float tw, float th,
                     uint32_t texmask, uint32_t flags) {

    if (count_ + reserved_ >= capacity_) {
        throw std::runtime_error("sprite batch overflow");
    }

    auto index = count_ + reserved_;
    count_++;

    setCoords(index, x, y, w, h);
    setColor(index,  r, g, b, a);
    setTextureCoords(index, tx, ty, tw, th);
    setTextureMask(index, texmask);
    setFlags(index, flags);

    modified_ = true;
}

void QuadBatch::push(const glm::vec4& rect) {
    set(&rect, &DEFAULT_COLOR, &DEFAULT_TEXTURE_COORDS, &DEFAULT_TEXTURE_MASK, &DEFAULT_FLAGS);
}

void QuadBatch::push(const glm::vec4& rect,
                       const glm::vec4& color,
                       const glm::vec4& texcoords,
                       uint32_t texmask,
                       uint32_t flags) {
    set(&rect, &color, &texcoords, &texmask, &flags);
}

void QuadBatch::store(size_t index,
                      float x, float y, float w, float h,
                      float r, float g, float b, float a,
                      float tx, float ty, float tw, float th,
                      uint32_t mask,
                      uint32_t flags) {

    if (index >= count_ + reserved_) {
        throw std::runtime_error("sprite batch index out of bounds");
    }

    setCoords(index, x, y, w, h);
    setColor(index,  r, g, b, a);
    setTextureCoords(index, tx, ty, tw, th);
    setTextureMask(index, mask);
    setFlags(index, flags);

    modified_ = true;
}

void QuadBatch::store(size_t index, const glm::vec4& rect) {
    set(&rect, &DEFAULT_COLOR, &DEFAULT_TEXTURE_COORDS, &DEFAULT_TEXTURE_MASK, &DEFAULT_FLAGS, index);
}

void QuadBatch::store(size_t index,
                      const glm::vec4& rect,
                      const glm::vec4& color,
                      const glm::vec4& texcoords,
                      uint32_t texmask,
                      uint32_t flags) {
    set(&rect, &color, &texcoords, &texmask, &flags, index);
}

///////////////////////////////////////////////////////////////////////////////
// Sprite Batch
///////////////////////////////////////////////////////////////////////////////

SpriteBatch SpriteBatch::make(size_t capacity) {
    SpriteBatch spriteBatch;
    spriteBatch.create(capacity);
    return spriteBatch;
}

void SpriteBatch::create(size_t capacity) {
    return VertexQueue::create(capacity);
}

void SpriteBatch::push(const Sprite& sprite) {
    set(sprite);
}

void SpriteBatch::store(size_t index, const Sprite& sprite) {
    set(sprite, index);
}

void SpriteBatch::set(const Sprite& sprite, size_t index) {

    auto texcoords = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    auto texmask = sprite.textureMask();
    auto flags = sprite.flags();

    VertexQueue::set(
        &sprite.coords(),
        &sprite.color(),
        &texcoords,
        &texmask,
        &flags,
        index
    );
}