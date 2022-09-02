/*
 * Main module
 */

#include "entity.h"

#include "gamekit/gamekit.h"

#include <iostream>
#include <cmath>
#include <execution>

using namespace gamekit;

static const bool parallelUpdates = false;
static const size_t numEntities = 500;

struct ShaderParams {
    float resolution_x;
    float resolution_y;
    float x_min;
    float x_max;
    float y_min;
    float y_max;
    float time;
    float time_delta;
    int32_t frame;
};

class Exec {

public:
    void onInit(Api& api) {

        auto& resources = api.resources();

        material_ = Material::make();

        material_.setDepthTesting(false);
        material_.setDepthWriting(false);
        material_.setBlendMode(BlendMode::Additive);

        material_.addShader(resources.getShader("shaders/shader.vert"));
        material_.addShader(resources.getShader("shaders/shader.frag"));
        material_.addTexture(resources.getTexture("particle.png"), 1);
        material_.addTexture(resources.getTexture("particle2.png"), 2);

        shaderParamsBuffer_ = Uniform<ShaderParams>::make(0);
        material_.addBuffer(shaderParamsBuffer_);

        api.addMaterial(material_);

        spriteBatch_ = QuadBatch::make(numEntities);

        for (auto& entity : entities_) {
            entity.initialize(0);
            if constexpr (parallelUpdates) {
                entity.batchIndex = spriteBatch_.reserve();
            }
        }

    }

    void onShutdown(Api& api) {
    }

    void onUpdate(Api& api) {

        auto deltaTime = api.deltaTime();
        auto absTime = api.absTime();

        float viewportWidth = api.metrics().width_f;
        float viewportHeight = api.metrics().height_f;

        auto& params = shaderParamsBuffer_.data();

        params.resolution_x = viewportWidth;
        params.resolution_y = viewportHeight;
        params.x_min = 0.0f;
        params.y_min = 0.0f;
        params.x_max = viewportWidth;
        params.y_max = viewportHeight;
        params.time = absTime;
        params.time_delta = deltaTime;
        params.frame++;
        shaderParamsBuffer_.copy();

        spriteBatch_.begin();

        if constexpr (parallelUpdates) {
            std::for_each(
                std::execution::par_unseq,
                entities_.begin(),
                entities_.end(),
                [&](auto&& entity)
                {
                    entity.update(deltaTime);
                    spriteBatch_.store(
                        entity.batchIndex,
                        entity.position.x, entity.position.y,
                        entity.size.x, entity.size.y,
                        entity.color.r, entity.color.g, entity.color.b, entity.color.a,
                        entity.texture_coords.x, entity.texture_coords.y, entity.texture_coords.z, entity.texture_coords.w,
                        entity.texture_mask, entity.flags
                    );
                }
            );
        } else {
            for (auto& entity : entities_) {
                entity.update(deltaTime);
                spriteBatch_.push(
                    entity.position.x, entity.position.y,
                    entity.size.x, entity.size.y,
                    entity.color.r, entity.color.g, entity.color.b, entity.color.a,
                    entity.texture_coords.x, entity.texture_coords.y, entity.texture_coords.z, entity.texture_coords.w,
                    entity.texture_mask, entity.flags
                );
            }
        }

        spriteBatch_.end();
    }

    void onDraw(Api& api) {
        spriteBatch_.draw();
    }

private:
    Material material_;
    Uniform<ShaderParams> shaderParamsBuffer_;
    QuadBatch spriteBatch_;
    std::array<Entity, numEntities> entities_;
};

int main(int argc, const char* argv[]) {
    Application<Exec, DefaultResourceDescriptor> app("Demo", 800, 600, 120);
    app.run();
    return 0;
}
