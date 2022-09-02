/*
 * Gamekit API
 */
#pragma once

#include <cstdint>
#include <vector>

#include "gamekit/metrics.h"
#include "gamekit/material.h"
#include "gamekit/frame.h"
#include "gamekit/resources.h"

namespace gamekit {

class Api {

    private:
        struct Context;

    public:
        void create();
        void destroy();

    public:
        float deltaTime() const;
        float absTime() const;

        void addMaterial(Material& material);
        void setMaterial(Material* material);
        Material* material();

        const Metrics& metrics() const;

        const Frame& currentFrame() const;

        Resources& resources();
        const Resources& resources() const;
        const ResourceDescriptor& resources(const std::string& id) const;

    private:
        Context* context_{nullptr};

};

} // namespace

extern const std::vector<gamekit::ResourceDescriptor>& getResourceDescriptors();
#define DefaultResourceDescriptor &getResourceDescriptors
