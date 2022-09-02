/*
 * Types
 */
#pragma once

#include <vulkan>
#include <vector>

#include <gamekit/types.h>

namespace gamekit {

class Material;

///////////////////////////////////////////////////////////////////////////////
// Buffer Type
///////////////////////////////////////////////////////////////////////////////

enum class BufferType {
    Unknown = 0x0,
    VertexBuffer = 0x1,
    IndexBuffer = 0x2,
    UniformBuffer = 0x3,
    ShaderStorageBuffer = 0x4,
    StagingBuffer = 0x5
};

class Buffer;

///////////////////////////////////////////////////////////////////////////////
// Buffer Object
///////////////////////////////////////////////////////////////////////////////

class BufferObject {
    public:
        static BufferObject make(BufferType bufferType, size_t size, uint32_t bufferUsage, uint32_t memoryUsage);

    public:
        BufferObject();
        BufferObject(BufferObject&& ref);
        BufferObject& operator=(BufferObject&& ref);
        BufferObject(const BufferObject&) = delete;
        BufferObject& operator=(const BufferObject&) = delete;
        virtual ~BufferObject();

    public:
        void create(BufferType bufferType, size_t size, uint32_t bufferUsage, uint32_t memoryUsage);
        void destroy();

    public:
        void bind() const;

    public:
        void* map() const;
        void* map(size_t ofs, size_t len) const;
        void unmap() const;
        void copy(const void* sourcePtr, size_t len) const;
        void copy(const void* sourcePtr) const;
        void copy(const BufferObject& source, size_t len) const;
        void copy(const BufferObject& source) const;

    public:
        [[nodiscard]] VkBuffer ptr() { return handle_; };
        [[nodiscard]] const VkBuffer ptr() const { return handle_; };
        [[nodiscard]] size_t size() const { return size_; }
        operator VkBuffer() const { return handle_; }

    private:
        BufferType bufferType_{BufferType::Unknown};
        size_t size_{0};
        VkBuffer handle_{nullptr};
        DeviceMemory memory_;
};


///////////////////////////////////////////////////////////////////////////////
// Buffer
///////////////////////////////////////////////////////////////////////////////

class Buffer {

    protected:
        enum Flags {
            None = 0x0,
            TransferSource = 0x100,
            TransferDest = 0x200,
        };

    protected:
        Buffer();
        Buffer(const Buffer&) = delete;
        Buffer& operator=(const Buffer&) = delete;
        Buffer(Buffer&& ref);
        Buffer& operator=(Buffer&& ref);
        virtual ~Buffer();

    public:
        void free();
        virtual void bind() const = 0;

    protected:
        void create(uint32_t binding, BufferType bufferType, size_t size);
        void destroy();

    public:
        [[nodiscard]] uint32_t binding() const { return binding_; }
        [[nodiscard]] BufferType bufferType() const { return bufferType_; }
        [[nodiscard]] size_t size() const { return size_; }
        [[nodiscard]] uint32_t flags() const { return flags_; }
        [[nodiscard]] VkDescriptorSetLayout descriptorSetLayout() const { return descriptorSetLayout_; }

    protected:
        void createDescriptorSetLayout();

    protected:
        uint32_t binding_{0};
        BufferType bufferType_{BufferType::Unknown};
        uint32_t flags_{None};
        size_t size_{0};
        std::vector<BufferObject> bufferObjects_;
        VkDescriptorSetLayout descriptorSetLayout_{nullptr};
};

///////////////////////////////////////////////////////////////////////////////
// Vertex Buffer
///////////////////////////////////////////////////////////////////////////////

class VertexBuffer : public Buffer {

    public:
        static VertexBuffer make(size_t size);

    public:
        void copy(const void* sourcePtr);
        void copy(const void* sourcePtr, size_t len);
        void bind() const override;
};

///////////////////////////////////////////////////////////////////////////////
// Index Buffer
///////////////////////////////////////////////////////////////////////////////

class IndexBuffer : public Buffer {

    public:
        static IndexBuffer make(size_t size);

    public:
        void copy(const void* sourcePtr);
        void bind() const override;
};

///////////////////////////////////////////////////////////////////////////////
// Uniform Buffer
///////////////////////////////////////////////////////////////////////////////

class UniformBuffer : public Buffer {

    public:
        static UniformBuffer make(uint32_t index, size_t size);

    public:
        void copy(const void* sourcePtr);
        void bind() const override;

    public:
        [[nodiscard]] BufferObject& allocFrameBuffer();
};

template <class T>
class Uniform : public UniformBuffer {

    public:
        static Uniform make(uint32_t index) {
            Uniform uniform;
            uniform.create(index, BufferType::UniformBuffer, sizeof(data_));
            return std::move(uniform);
        }

    public:
        void copy() {
            UniformBuffer::copy(&data_);
        }

    public:
        [[nodiscard]] T& data() { return data_; }
        [[nodiscard]] const T& data() const { return data_; }

    private:
        T data_;
};

///////////////////////////////////////////////////////////////////////////////
// Storage Buffer
///////////////////////////////////////////////////////////////////////////////

class ShaderStorageBuffer : public Buffer {

    public:
        static ShaderStorageBuffer make(uint32_t index, size_t size);

    public:
        void copy(const void* sourcePtr);
        void bind() const override;

    public:
        [[nodiscard]] BufferObject& allocFrameBuffer();
};

template <class T>
class ShaderStorage : public ShaderStorageBuffer {
    public:
        static ShaderStorage make(uint32_t index) {
            ShaderStorage shaderStorage;
            shaderStorage.create(index, BufferType::ShaderStorageBuffer, sizeof(data_));
            return std::move(shaderStorage);
        }

    public:
        void copy() {
            ShaderStorage::copy(&data_);
        }

    public:
        [[nodiscard]] T& data() { return data_; }
        [[nodiscard]] const T& data() const { return data_; }

    private:
        T data_;
};

///////////////////////////////////////////////////////////////////////////////
// Push Constants Base
///////////////////////////////////////////////////////////////////////////////

class PushConstantsBase {

    public:
        PushConstantsBase(void* raw_ptr, size_t size) : raw_ptr_(raw_ptr), size_(size) {}
        void attachToMaterial(Material* material) { material_ = material; }

    public:
        [[nodiscard]] const void* raw_ptr() const { return raw_ptr_; }
        [[nodiscard]] size_t size() const { return size_; }
        [[nodiscard]] Material* material() const { return material_; }

    public:
        void push() const;

    protected:
        size_t size_{0};
        void* raw_ptr_{nullptr};
        Material* material_{nullptr};
};

///////////////////////////////////////////////////////////////////////////////
// Push Constants<T>
///////////////////////////////////////////////////////////////////////////////

template <typename T>
class PushConstants : public PushConstantsBase {

    public:
        PushConstants() : PushConstantsBase(&data_, sizeof(data_)) {}

    public:
        [[nodiscard]] T& data() { return data_; }
        [[nodiscard]] const T& data() const { return data_; }

    private:
        T data_;

};

} // namespace
