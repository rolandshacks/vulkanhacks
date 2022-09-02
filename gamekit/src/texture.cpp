/*
 * Utilities
 */

#include <vulkan>

#include "gamekit/texture.h"
#include "gamekit/device.h"

#include <stdexcept>

using namespace gamekit;

Texture Texture::make(const ResourceDescriptor& resourceDescriptor) {
    Texture texture;
    texture.create(resourceDescriptor);
    return texture;
}

Texture Texture::make(const std::string& filename) {
    Texture texture;
    texture.create(filename);
    return texture;
}

Texture Texture::make(const Image& image) {
    Texture texture;
    texture.create(image);
    return texture;
}

Texture::Texture() {
}

Texture::Texture(Texture&& ref) {
    image_ = std::move(ref.image_);
    imageView_ = std::move(ref.imageView_);
    sampler_ = std::move(ref.sampler_);
    width_ = ref.width_; ref.width_ = 0;
    height_ = ref.height_; ref.height_ = 0;
}

Texture& Texture::operator=(Texture&& ref) {
    if (this == &ref) {
        return *this;
    }

    free();

    image_ = std::move(ref.image_);
    imageView_ = std::move(ref.imageView_);
    sampler_ = std::move(ref.sampler_);
    width_ = ref.width_; ref.width_ = 0;
    height_ = ref.height_; ref.height_ = 0;

    return *this;
}

Texture::~Texture() {
    free();
}

void Texture::create(const ResourceDescriptor& resourceDescriptor) {
    image_ = Image::make(resourceDescriptor);
    imageView_ = ImageView::make(image_);
    sampler_ = Sampler::make();
    width_ = image_.width();
    height_ = image_.height();
}

void Texture::create(const std::string& filename) {
    filename_ = filename;
    image_ = Image::make(filename);
    imageView_ = ImageView::make(image_);
    sampler_ = Sampler::make();
    width_ = image_.width();
    height_ = image_.height();
}

void Texture::create(const Image& image) {
    imageView_ = ImageView::make(image);
    sampler_ = Sampler::make();
    width_ = image.width();
    height_ = image.height();
}

void Texture::free() {
    destroy();
    width_ = 0;
    height_ = 0;
}

void Texture::destroy() {
    if (!Device::globalInstance()) return;
    sampler_.destroy();
    imageView_.destroy();
    image_.destroy();
}
