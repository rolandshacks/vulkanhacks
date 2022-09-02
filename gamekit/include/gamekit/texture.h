/*
 * Types
 */
#pragma once

#include <vulkan>
#include <vector>

#include <gamekit/types.h>

namespace gamekit {

class Texture {

    public:
        static Texture make(const ResourceDescriptor& resourceDescriptor);
        static Texture make(const std::string& filename);
        static Texture make(const Image& image);

    public:
        Texture();
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&& ref);
        Texture& operator=(Texture&& ref);
        virtual ~Texture();

    public:
        void free();

    protected:
        void create(const ResourceDescriptor& filename);
        void create(const std::string& filename);
        void create(const Image& image);
        void destroy();

    public:
        [[nodiscard]] const Image& image() const { return image_; }
        [[nodiscard]] const ImageView& imageView() const { return imageView_; }
        [[nodiscard]] const Sampler& sampler() const { return sampler_; }
        [[nodiscard]] int width() const { return width_; }
        [[nodiscard]] int height() const { return height_; }

    protected:
        std::string filename_;
        Image image_;
        ImageView imageView_;
        Sampler sampler_;
        int width_{0};
        int height_{0};

};

} // namespace
