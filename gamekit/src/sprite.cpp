/*
 * Quad batch
 */

#include <vulkan>

#include <gamekit/device.h>
#include <gamekit/vertex.h>
#include <gamekit/texture.h>
#include <gamekit/buffer.h>
#include "gamekit/sprite.h"

#include <array>
#include <stdexcept>
#include <glm/glm.hpp>

using namespace gamekit;

static const glm::vec4 DEFAULT_COLOR { 1.0f, 1.0f, 1.0f, 1.0f };
static const glm::vec4 DEFAULT_TEXTURE_COORDS { 0.0f, 0.0f, 1.0f, 1.0f };
static const uint32_t DEFAULT_TEXTURE_MASK = 0x1;
static const uint32_t DEFAULT_FLAGS = 0x0;


Quad Quad::make() {
    Quad quad;
    quad.create();
    return quad;
}

void Quad::create() {
    auto ptr = indices_.data();
    *(ptr++) = static_cast<uint16_t>(2);
    *(ptr++) = static_cast<uint16_t>(1);
    *(ptr++) = static_cast<uint16_t>(0);
    *(ptr++) = static_cast<uint16_t>(0);
    *(ptr++) = static_cast<uint16_t>(3);
    *(ptr++) = static_cast<uint16_t>(2);

    indexBuffer_ = IndexBuffer::make(indices_.size() * sizeof(uint16_t));
    indexBuffer_.copy(indices_.data());

    vertexBuffer_ = VertexBuffer::make(vertices_.size() * sizeof(Vertex));

    setCoords(0.0f, 0.0f, 100.0f, 100.0f);
    setTextureCoords(DEFAULT_TEXTURE_COORDS);
    setColor(DEFAULT_COLOR);
    setTextureMask(DEFAULT_TEXTURE_MASK);
    setFlags(DEFAULT_FLAGS);
}

void Quad::draw() {
    update();

    auto device = Device::globalInstance();
    auto numIndices = indices_.size();
    vertexBuffer_.bind();
    indexBuffer_.bind();
    device->drawIndexed(numIndices);
}

void Quad::update() {

    if (!modified_) {
        return;
    }

    auto x0 = coords_.x;
    auto y0 = coords_.y;
    auto x1 = x0 + coords_.z;
    auto y1 = y0 + coords_.w;
    auto z = 0.0f;

    auto u0 = texcoords_.x;
    auto v0 = texcoords_.y;
    auto u1 = u0 + texcoords_.z;
    auto v1 = v0 + texcoords_.w;

    auto r = color_.r;
    auto g = color_.g;
    auto b = color_.b;
    auto a = color_.a;

    auto v = vertices_.data();

    v->setPos(x0, y0, z);
    v->setTexcoord(u0, v0);
    v->setColor(r, g, b, a);
    v->setTexmask(texmask_);
    v->setFlags(flags_);
    v++;

    v->setPos(x1, y0, z);
    v->setTexcoord(u1, v0);
    v->setColor(r, g, b, a);
    v->setTexmask(texmask_);
    v->setFlags(flags_);
    v++;

    v->setPos(x1, y1, z);
    v->setTexcoord(u1, v1);
    v->setColor(r, g, b, a);
    v->setTexmask(texmask_);
    v->setFlags(flags_);
    v++;


    v->setPos(x0, y1, z);
    v->setTexcoord(u0, v1);
    v->setColor(r, g, b, a);
    v->setTexmask(texmask_);
    v->setFlags(flags_);
    v++;

    modified_ = false;
    vertexBuffer_.copy(vertices_.data(), sizeof(Vertex) * vertices_.size());
}

inline void Quad::setPosition(float x, float y) {
    coords_.x = x;
    coords_.y = y;
    modified_ = true;
}

inline void Quad::setSize(float w, float h) {
    coords_.z = w;
    coords_.w = h;
    modified_ = true;
}

inline void Quad::setCoords(float x, float y, float w, float h) {
    coords_.x = x;
    coords_.y = y;
    coords_.z = w;
    coords_.w = h;
    modified_ = true;
}

inline void Quad::setCoords(const glm::vec4& coords) {
    coords_ = coords;
    modified_ = true;
}

inline void Quad::setColor(const glm::vec4& color) {
    color_ = color;
    modified_ = true;
}

inline void Quad::setTextureCoords(const glm::vec4& texture_coords) {
    texcoords_ = texture_coords;
    modified_ = true;
}

inline void Quad::setTextureMask(uint32_t texture_mask) {
    texmask_ = texture_mask;
    modified_ = true;
}

inline void Quad::setFlags(uint32_t flags) {
    flags_ = flags;
    modified_ = true;
}

///////////////////////////////////////////////////////////////////////////////
// Sprite
///////////////////////////////////////////////////////////////////////////////

Sprite Sprite::make() {
    Sprite sprite;
    sprite.create();
    return sprite;
}

void Sprite::create() {
}
