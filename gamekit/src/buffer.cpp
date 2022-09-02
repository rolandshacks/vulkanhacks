/*
 * Utilities
 */

#include <vulkan>

#include "gamekit/material.h"
#include "gamekit/buffer.h"
#include "gamekit/device.h"

#include <stdexcept>
#include <cassert>
#include <cstring>

using namespace gamekit;

Buffer::Buffer() {
}

Buffer::Buffer(Buffer&& ref) {
    binding_ = ref.binding_; ref.binding_ = 0;
    bufferType_ = ref.bufferType_; ref.bufferType_ = BufferType::Unknown;
    flags_ = ref.flags_; ref.flags_ = 0;
    size_ = ref.size_; ref.size_ = 0;
    bufferObjects_ = std::move(ref.bufferObjects_);
    descriptorSetLayout_ = ref.descriptorSetLayout_; ref.descriptorSetLayout_ = nullptr;
}

Buffer& Buffer::operator=(Buffer&& ref) {
    if (this == &ref) {
        return *this;
    }

    free();

    binding_ = ref.binding_; ref.binding_ = 0;
    bufferType_ = ref.bufferType_; ref.bufferType_ = BufferType::Unknown;
    flags_ = ref.flags_; ref.flags_ = 0;
    size_ = ref.size_; ref.size_ = 0;
    bufferObjects_ = std::move(ref.bufferObjects_);
    descriptorSetLayout_ = ref.descriptorSetLayout_; ref.descriptorSetLayout_ = nullptr;

    return *this;
}

Buffer::~Buffer() {
    free();
}

void Buffer::create(uint32_t binding, BufferType bufferType, size_t size) {
    binding_ = binding;
    bufferType_ = bufferType;
    size_ = size;
}

void Buffer::free() {
    destroy();
}

void Buffer::destroy() {

    if (!Device::globalInstance()) return;

    auto device = Device::globalHandle();

    for (auto& bufferObject : bufferObjects_) {
        bufferObject.destroy();
    }
    bufferObjects_.clear();

    if (nullptr != descriptorSetLayout_) {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout_, nullptr);
        descriptorSetLayout_ = nullptr;
    }

    size_ = 0;
}

void Buffer::createDescriptorSetLayout() {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    VkDescriptorType descriptorType{};

    switch (bufferType_) {
        case BufferType::UniformBuffer: descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
        case BufferType::ShaderStorageBuffer: descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
        default: return;
    }

    VkDescriptorSetLayoutBinding binding{};
    binding.descriptorType = descriptorType;
    binding.binding = 0; // set #0
    binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    binding.descriptorCount = 1;
    binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

///////////////////////////////////////////////////////////////////////////////
// Buffer Object
///////////////////////////////////////////////////////////////////////////////

/*
    BufferType bufferType_{BufferType::Unknown};
    size_t size_{0};
    VkBuffer handle_{nullptr};
    DeviceMemory memory_;
*/


BufferObject BufferObject::make(BufferType bufferType, size_t size, uint32_t bufferUsage, uint32_t memoryUsage) {
    BufferObject bufferObject;
    bufferObject.create(bufferType, size, bufferUsage, memoryUsage);
    return bufferObject;
}

BufferObject::BufferObject() {}

BufferObject::BufferObject(BufferObject&& ref) {
    bufferType_ = ref.bufferType_; ref.bufferType_ = BufferType::Unknown;
    size_ = ref.size_; ref.size_ = 0;
    handle_ = ref.handle_; ref.handle_ = nullptr;
    memory_ = std::move(ref.memory_);
}

BufferObject& BufferObject::operator=(BufferObject&& ref) {
    if (this == &ref) {
        return *this;
    }

    destroy();
    bufferType_ = ref.bufferType_; ref.bufferType_ = BufferType::Unknown;
    size_ = ref.size_; ref.size_ = 0;
    handle_ = ref.handle_; ref.handle_ = nullptr;
    memory_ = std::move(ref.memory_);

    return *this;
}

BufferObject::~BufferObject() {
    destroy();
}

void BufferObject::create(BufferType bufferType, size_t size, uint32_t bufferUsage, uint32_t memoryUsage) {

    destroy();

    this->bufferType_ = bufferType;
    this->size_ = size;

    auto device = Device::globalHandle();
    assert(nullptr != device);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size_;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = bufferUsage;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &handle_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    if (0x0 != memoryUsage) {
        VkMemoryRequirements memRequirements{};
        vkGetBufferMemoryRequirements(device, handle_, &memRequirements);

        memory_ = DeviceMemory::make(memRequirements.size,
                                     memRequirements.memoryTypeBits,
                                     memoryUsage);

        vkBindBufferMemory(device, handle_, memory_, 0);
    }

}

void BufferObject::destroy() {
    if (nullptr == handle_) return;

    auto device = Device::globalHandle();

    memory_.destroy();
    vkDestroyBuffer(device, handle_, nullptr);
    handle_ = nullptr;

    size_ = 0;
}

void BufferObject::bind() const {
    const auto& frame = Device::globalInstance()->currentFrame();
    const auto& commandBuffer = frame.commandBuffer;

    if (nullptr == handle_) return;

    switch (bufferType_) {
        case BufferType::VertexBuffer: {
            VkDeviceSize offset = 0;
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &handle_, &offset);
            break;
        }
        case BufferType::IndexBuffer: {
            vkCmdBindIndexBuffer(commandBuffer, handle_, 0, VK_INDEX_TYPE_UINT16);
            break;
        }
        case BufferType::UniformBuffer: {
            // nothing to bind
            break;
        }
        case BufferType::ShaderStorageBuffer: {
            // nothing to bind
            break;
        }
        default: {
            break;
        }
    }

}

void* BufferObject::map() const {
    return map(0, size_);
}

void* BufferObject::map(size_t ofs, size_t len) const {
    return memory_.map(ofs, len);
}

void BufferObject::unmap() const {
    return memory_.unmap();
}

void BufferObject::copy(const void* source_ptr) const {
    copy(source_ptr, size_);
}

void BufferObject::copy(const void* source_ptr, size_t len) const {
    void* dest_ptr = map();
    std::memcpy(dest_ptr, source_ptr, len);
    unmap();
}

void BufferObject::copy(const BufferObject& src) const {
    copy(src, src.size_);
}

void BufferObject::copy(const BufferObject& src, size_t len) const {

    auto srcBuffer = src.handle_;
    auto destBuffer = handle_;

    if (nullptr == srcBuffer) return;

    auto deviceObj = Device::globalInstance();
    assert(nullptr != deviceObj);

    auto device = deviceObj->handle();
    assert(nullptr != device);

    auto commandPool = deviceObj->commandPool();
    assert(nullptr != commandPool);

    auto graphicsQueue = deviceObj->graphicsQueue();
    assert(nullptr != graphicsQueue);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = src.size_;

    vkCmdCopyBuffer(commandBuffer, srcBuffer, destBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

///////////////////////////////////////////////////////////////////////////////
// Vertex Buffer
///////////////////////////////////////////////////////////////////////////////

VertexBuffer VertexBuffer::make(size_t size) {
    VertexBuffer buffer;
    buffer.create(0, BufferType::VertexBuffer, size);

    // device memory
    buffer.bufferObjects_.emplace_back(BufferObject::make(
        BufferType::VertexBuffer,
        size,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        DeviceMemory::DeviceLocalMemory)
    );

    // staging memory
    buffer.bufferObjects_.emplace_back(BufferObject::make(
        BufferType::StagingBuffer,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        DeviceMemory::HostVisibleMemory | DeviceMemory::HostCoherentMemory)
    );

    return std::move(buffer);
}

void VertexBuffer::copy(const void* sourcePtr) {
    copy(sourcePtr, size_);
}

void VertexBuffer::copy(const void* sourcePtr, size_t len) {
    assert(bufferObjects_.size() >=2 );
    bufferObjects_[1].copy(sourcePtr, len);              // copy to staging buffer
    bufferObjects_[0].copy(bufferObjects_[1], len);      // copy to device memory
}

void VertexBuffer::bind() const {
    assert(bufferObjects_.size() >=1 );
    bufferObjects_[0].bind();
}

///////////////////////////////////////////////////////////////////////////////
// Index Buffer
///////////////////////////////////////////////////////////////////////////////

IndexBuffer IndexBuffer::make(size_t size) {
    IndexBuffer buffer;
    buffer.create(0, BufferType::IndexBuffer, size);

    // device memory
    buffer.bufferObjects_.emplace_back(BufferObject::make(
        BufferType::IndexBuffer,
        size,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        DeviceMemory::DeviceLocalMemory)
    );

    // staging memory
    buffer.bufferObjects_.emplace_back(BufferObject::make(
        BufferType::StagingBuffer,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        DeviceMemory::HostVisibleMemory | DeviceMemory::HostCoherentMemory)
    );

    return std::move(buffer);
}

void IndexBuffer::copy(const void* sourcePtr) {
    assert(bufferObjects_.size() >=2 );
    bufferObjects_[1].copy(sourcePtr);              // copy to staging buffer
    bufferObjects_[0].copy(bufferObjects_[1]);      // copy to device memory
}

void IndexBuffer::bind() const {
    assert(bufferObjects_.size() >=1 );
    bufferObjects_[0].bind();
}

///////////////////////////////////////////////////////////////////////////////
// Uniform Buffer
///////////////////////////////////////////////////////////////////////////////

UniformBuffer UniformBuffer::make(uint32_t index, size_t size) {
    UniformBuffer buffer;
    buffer.create(index, BufferType::UniformBuffer, size);
    return std::move(buffer);
}

void UniformBuffer::copy(const void* sourcePtr) {
    const auto& frame = Device::globalInstance()->currentFrame();
    const auto& bufferObject = bufferObjects_[frame.index];
    bufferObject.copy(sourcePtr);
}

void UniformBuffer::bind() const {
    const auto& frame = Device::globalInstance()->currentFrame();
    const auto& bufferObject = bufferObjects_[frame.index];
    bufferObject.bind();
}

BufferObject& UniformBuffer::allocFrameBuffer() {
    // alloc per-frame uniform buffer object
    auto& bufferObject = bufferObjects_.emplace_back(BufferObject::make(
        BufferType::UniformBuffer,
        size_,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        DeviceMemory::HostVisibleMemory | DeviceMemory::HostCoherentMemory)
    );

    return bufferObject;
}

///////////////////////////////////////////////////////////////////////////////
// Storage Buffer
///////////////////////////////////////////////////////////////////////////////

ShaderStorageBuffer ShaderStorageBuffer::make(uint32_t index, size_t size) {
    ShaderStorageBuffer buffer;
    buffer.create(index, BufferType::ShaderStorageBuffer, size);
    return std::move(buffer);
}

void ShaderStorageBuffer::copy(const void* sourcePtr) {
    const auto& frame = Device::globalInstance()->currentFrame();
    const auto& bufferObject = bufferObjects_[frame.index];
    bufferObject.copy(sourcePtr);
}

void ShaderStorageBuffer::bind() const {
    const auto& frame = Device::globalInstance()->currentFrame();
    const auto& bufferObject = bufferObjects_[frame.index];
    bufferObject.bind();
}

BufferObject& ShaderStorageBuffer::allocFrameBuffer() {
    // alloc per-frame storage buffer object
    auto& bufferObject = bufferObjects_.emplace_back(BufferObject::make(
        BufferType::ShaderStorageBuffer,
        size_,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        DeviceMemory::HostVisibleMemory | DeviceMemory::HostCoherentMemory)
    );

    return bufferObject;
}

void PushConstantsBase::push() const {
    material_->updatePushConstants(*this);
}
