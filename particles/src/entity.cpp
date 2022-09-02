/*
 * Main module
 */

#include "entity.h"

#include "gamekit/utilities.h"
#include "gamekit/device.h"

#include <iostream>

using namespace gamekit;

static inline glm::vec2 vector_distance(const glm::vec2& v1, const glm::vec2& v2) {
    return glm::vec2(v2.x-v1.x, v2.y-v1.y);
}

static inline float vector_len(const glm::vec2& v) {
    return std::sqrt(v.x*v.x + v.y*v.y);
}

static void vector_normalize(glm::vec2& v) {
    auto l = vector_len(v);
    if (l > 0.0f) {
        v.x /= l;
        v.y /= l;
    }
}

static glm::vec4 hsv_to_rgb(float h, float s, float v) {

    if (h >= 360.0f) h = 0.0f; else h /= 60.0f;
    float fract = h - std::floor(h);
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * fract);
    float t = v * (1.0f - s * (1.0f - fract));

    float r, g, b;

    if (0.0f <= h && h < 1.0f)
        { r = v; g = t; b = p; }
    else if (1.0f <= h && h < 2.0f)
        { r = q, g = v, b = p; }
    else if (2.0f <= h && h < 3.0f)
        { r = p; g = v, b = t; }
    else if (3.0f <= h && h < 4.0f)
        { r = p; g = q, b = v; }
    else if (4.0f <= h && h < 5.0f)
        { r = t; g = p; b = v; }
    else if (5.0f <= h && h < 6.0f)
        { r = v; g = p; b = q; }
    else
        { r = 0.0f; g = 0.0f; b = 0.0f; }

    return glm::vec4(r, g, b, 1.0f);
}

void Entity::initialize(int frameCounter) {

    time_to_live = Random::getFloat(2.0, 5.0);

    auto device = Device::globalInstance();

    auto x = Random::getFloat(0.0f, (float) device->metrics().width_f);
    auto y = Random::getFloat(0.0f, (float) device->metrics().height_f);

    if (frameCounter == 0) {
        auto hue = Random::getFloat(0.0, 360.0);
        color = hsv_to_rgb(hue, 1.0f, 0.5f);
    }

    target = glm::vec2(x, y);

    texture_mask = (Random::getFloat() > 0.5) ? 2 : 1;

}

void Entity::update(float deltaTime) {
    //let metrics = api.get_metrics();

    auto distance = vector_distance(position, target);
    auto len = vector_len(distance);
    if (len > 0.0f) {
        distance.x /= len;
        distance.y /= len;
    }

    if (time_to_live <= 0.0f || len < 100.0f) {
        initialize(1);
        return;
    }

    if (time_to_live > 0.0f) {
        time_to_live -= deltaTime;
    }

    velocity.x += distance.x * 5.0f * deltaTime;
    velocity.y += distance.y * 5.0f * deltaTime;
    velocity.y += 5.0f * deltaTime;

    vector_normalize(velocity);

    auto speed = 500.0f;

    position.x += velocity.x * speed * deltaTime;
    position.y += velocity.y * speed * deltaTime;

    float min_x = 0.0f;
    float max_x = (float) Device::globalInstance()->metrics().width_f - size.x;
    float min_y = 0.0f;
    float max_y = (float) Device::globalInstance()->metrics().height_f - size.y;

    if (position.y >= max_y) {
        position.y = max_y;
        velocity.y = -std::abs(velocity.y * 2.0f);
    } else if (position.y <= min_y) {
        position.y = min_y;
        velocity.y = std::abs(velocity.y);
    }

    if (position.x >= max_x) {
        position.x = max_x;
        velocity.x = -std::abs(velocity.x);
    } else if (position.x <= min_x) {
        position.x= min_x;
        velocity.x = std::abs(velocity.x);
    }

}
