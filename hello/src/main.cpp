/*
 * Vulkan Hello, world!
 */

#include "gamekit/gamekit.h"

#include <iostream>
#include <cmath>
#include <execution>
#include <functional>
#include <optional>

using namespace gamekit;

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
        textureRef_ = material_.addTexture(resources.getTexture("bitmap.png"), 1);

        shaderParamsBuffer_ = Uniform<ShaderParams>::make(0);
        material_.addBuffer(shaderParamsBuffer_);
        api.addMaterial(material_);

        quad_ = Quad::make();

    }

    void onShutdown(Api& api) {
    }

    void onUpdate(Api& api) {

        auto deltaTime = api.deltaTime();
        auto absTime = api.absTime();

        float screenWidth = api.metrics().width_f;
        float screenHeight = api.metrics().height_f;

        auto& params = shaderParamsBuffer_.data();

        params.resolution_x = screenWidth;
        params.resolution_y = screenHeight;
        params.x_min = 0.0f;
        params.y_min = 0.0f;
        params.x_max = screenWidth;
        params.y_max = screenHeight;
        params.time = absTime;
        params.time_delta = deltaTime;
        params.frame++;
        shaderParamsBuffer_.copy();

        if (nullptr != textureRef_) {
            float quadWidth = (float) textureRef_->width();
            float quadHeight = (float) textureRef_->height();
            float quadRatio = quadWidth / quadHeight;

            if (quadWidth > screenWidth) {
                quadWidth = screenWidth;
                quadHeight = quadWidth / quadRatio;
            }

            if (quadHeight > screenHeight) {
                quadHeight = screenHeight;
                quadWidth = quadHeight * quadRatio;
            }

            quad_.setCoords((screenWidth-quadWidth)/2.0f, (screenHeight-quadHeight)/2.0f, quadWidth, quadHeight);
        }

    }

    void onDraw(Api& api) {
        quad_.draw();
    }

private:
    Material material_;
    Uniform<ShaderParams> shaderParamsBuffer_;
    Quad quad_;
    const Texture* textureRef_{nullptr};
};

int main(int argc, const char* argv[]) {
    Application<Exec, DefaultResourceDescriptor> app("Hello", 800, 600, 120);
    app.run();
    return 0;
}
