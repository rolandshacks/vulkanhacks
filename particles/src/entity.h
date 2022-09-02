/*
 * Entity
 */

#include <glm/glm.hpp>

namespace gamekit {

struct Entity {
    glm::vec2 position{0.0f, 0.0f};
    glm::vec2 size{24.0f, 24.0f};
    glm::vec4 color{1.0f, 1.0f, 1.0f, 0.5f};
    glm::vec4 texture_coords{0.0f, 0.0f, 1.0f, 1.0f};
    uint32_t texture_mask{1};
    uint32_t flags{0x0};
    glm::vec2 target{0.0f, 0.0f};
    glm::vec2 velocity{0.0f, 0.0f};
    float time_to_live{0.0f};
    size_t batchIndex{0};

    void initialize(int frameCounter);
    void update(float deltaTime);
};

} // namespace
