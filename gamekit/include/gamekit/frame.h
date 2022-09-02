/*
 * Device
 */
#pragma once

#include <vulkan>

#include "gamekit/types.h"
#include "gamekit/buffer.h"

#include <vector>

namespace gamekit {

class Frame {
    public:
        uint32_t index;
        CommandBuffer commandBuffer;
        Semaphore imageAvailable;
        Semaphore renderFinished;
        Fence commandBuffersCompleted;

    public:
        void create(uint32_t index);
        void destroy();
};

} // namespace
