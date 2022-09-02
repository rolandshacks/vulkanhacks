/*
 * Resources
 */
#pragma once

#include "gamekit/primitives.h"
#include "gamekit/types.h"
#include "gamekit/texture.h"

#include <string>
#include <vector>
#include <unordered_map>

namespace gamekit {

class Resources {
    public:
        Resources() {}
        Resources(const Resources&) = delete;
        Resources& operator=(const Resources&) = delete;
        Resources(const Resources&&) = delete;
        Resources& operator=(const Resources&&) = delete;
        ~Resources() { destroy(); }

    public:
        void create(const std::vector<ResourceDescriptor>& resourceDescriptors);
        void destroy();

    public:
        const ResourceDescriptor& get(const std::string& id) const;
        const Shader& getShader(const std::string& id);
        const Image& getImage(const std::string& id);
        const Texture& getTexture(const std::string& id);

    private:
        std::unordered_map<std::string, ResourceDescriptor> descriptors_;
        std::unordered_map<std::string, Shader> shaders_;
        std::unordered_map<std::string, Image> images_;
        std::unordered_map<std::string, Texture> textures_;
};

} // namespace
