/*
 * Resources
 */

#include "gamekit/resources.h"
#include "gamekit/utilities.h"

#include <cstdlib>
#include <vector>
#include <string>
#include <stdexcept>
#include <cassert>

using namespace gamekit;

void Resources::create(const std::vector<gamekit::ResourceDescriptor>& resourceDescriptors) {
    for (const auto& descriptor : resourceDescriptors) {
        auto id = descriptor.name;
        descriptors_[id] = descriptor;
    }
}

void Resources::destroy() {
    shaders_.clear();
    images_.clear();
    textures_.clear();
}

const ResourceDescriptor& Resources::get(const std::string& id) const {
    auto it = descriptors_.find(id);
    if (it == descriptors_.end()) {
        throw std::runtime_error(Format::str("could not find resource: ", id.c_str()));
    }
    return it->second;
}

const Shader& Resources::getShader(const std::string& id) {
    auto it = shaders_.find(id);
    if (it == shaders_.end()) {
        auto res = shaders_.emplace(std::make_pair(id, Shader::make(get(id))));
        return res.first->second;
    }
    return it->second;
}

const Image& Resources::getImage(const std::string& id) {
    auto it = images_.find(id);
    if (it == images_.end()) {
        auto res = images_.emplace(std::make_pair(id, Image::make(get(id))));
        return res.first->second;
    }
    return it->second;
}

const Texture& Resources::getTexture(const std::string& id) {
    auto it = textures_.find(id);
    if (it == textures_.end()) {
        auto res = textures_.emplace(std::make_pair(id, Texture::make(getImage(id))));
        return res.first->second;
    }
    return it->second;
}
