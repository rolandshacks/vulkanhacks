/*
 * Frame
 */

#include <vulkan>

#include "gamekit/types.h"
#include "gamekit/frame.h"

using namespace gamekit;

void Frame::create(uint32_t index) {
    this->index = index;
    commandBuffer  = CommandBuffer::make();
    imageAvailable = Semaphore::make();
    renderFinished = Semaphore::make();
    commandBuffersCompleted = Fence::make(true);
}

void Frame::destroy() {
    index = 0;
    commandBuffer.destroy();
    imageAvailable.destroy();
    renderFinished.destroy();
    commandBuffersCompleted.destroy();
}
