/*
 * Types
 */
#pragma once

#include <vulkan>

#include <gamekit/device.h>
#include <gamekit/vertex.h>
#include <gamekit/buffer.h>

#include <array>
#include <glm/glm.hpp>

namespace gamekit {

class Quad {

    public:
        static Quad make();
        virtual void create();
        virtual void draw();

    public:
        const glm::vec4& coords() const { return coords_; }
        const glm::vec4& color() const { return color_; }
        const glm::vec4& textureCoords() const { return texcoords_; }
        const uint32_t& textureMask() const { return texmask_; }
        const uint32_t& flags() const { return texmask_; }

    public:
        inline void setPosition(float x, float y);
        inline void setSize(float w, float h);
        inline void setCoords(float x, float y, float w, float h);
        inline void setCoords(const glm::vec4& coords);
        inline void setColor(const glm::vec4& color);
        inline void setTextureCoords(const glm::vec4& texture_coords);
        inline void setTextureMask(uint32_t texture_mask);
        inline void setFlags(uint32_t flags);

    protected:
        virtual void update();

    private:
        std::array<Vertex, 4> vertices_;
        std::array<uint16_t, 6> indices_;
        VertexBuffer vertexBuffer_;
        IndexBuffer indexBuffer_;

    protected:
        bool modified_{false};
        glm::vec4 coords_;
        glm::vec4 color_;
        glm::vec4 texcoords_;
        uint32_t texmask_{0};
        uint32_t flags_{0x0};
};

class Sprite {
    public:
        static Sprite make();
        void create();

    protected:
        void update();

    public:
        inline void setCoords(float x, float y, float w, float h) { coords_ = glm::vec4(x, y, w, h); };
        inline void setCoords(const glm::vec4& coords) { coords_ = coords; };
        inline const glm::vec4& coords() const { return coords_; }
        inline void setColor(const glm::vec4& color) { color_ = color; };
        inline const glm::vec4& color() const { return color_; }
        inline void setTextureMask(uint32_t textureMask) { texmask_ = textureMask; };
        inline uint32_t textureMask() const { return texmask_; }
        inline void setFlags(uint32_t flags) { flags_ = flags; };
        inline uint32_t flags() const { return flags_; }

    public:
        void setFrame(int frame) { frame_ = frame; }
        int frame() const { return frame_; }

    private:
        glm::vec4 coords_;
        glm::vec4 color_;
        uint32_t texmask_{0};
        uint32_t flags_{0x0};
        int frame_{0};
};

} // namespace
