/*
 * Types
 */
#pragma once

#include "gamekit/reference.h"
#include "gamekit/primitives.h"

#include <vulkan>

#include <string>
#include <vector>
#include <array>

namespace gamekit {

///////////////////////////////////////////////////////////////////////////////
// Semaphore
///////////////////////////////////////////////////////////////////////////////

class Semaphore {
    public:
        static Semaphore make();
        void destroy() { handle_.free(); }

    public:
        [[nodiscard]] VkSemaphore ptr() const { return handle_.ptr(); }
        operator VkSemaphore() const { return handle_.ptr(); }

    private:
        Reference<VkSemaphore> handle_;
};

template <> void Reference<VkSemaphore>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Fence
///////////////////////////////////////////////////////////////////////////////

class Fence {
    public:
        static Fence make(bool signaled=false);
        void destroy() { handle_.free(); }

    public:
        VkResult wait(nanosecond_t timeout=-1) const;
        VkResult waitAndReset(nanosecond_t timeout=-1) const;
        VkResult reset() const;

    public:
        [[nodiscard]] VkFence ptr() const { return handle_.ptr(); }
        operator VkFence() const { return handle_.ptr(); }

    private:
        Reference<VkFence> handle_;
};

template <> void Reference<VkFence>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Command Buffer
///////////////////////////////////////////////////////////////////////////////

class CommandBuffer {
    public:
        static CommandBuffer make();
        void destroy() {
            handle_.free();
        }

    public:
        VkResult reset();
        VkResult begin();
        VkResult end();

    public:
        [[nodiscard]] VkCommandBuffer ptr() const { return handle_.ptr(); }
        operator VkCommandBuffer() const { return handle_.ptr(); }

    private:
        Reference<VkCommandBuffer> handle_;
};

template <> void Reference<VkCommandBuffer>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Shader
///////////////////////////////////////////////////////////////////////////////

enum class ShaderType {
    Unknown = 0x0,
    VertexShader = 0x1,
    FragmentShader = 0x2
};

struct ShaderDescriptor {
    const void* code;
    size_t codeSize;
    ShaderType type;
};

class Shader {

    public:
        static Shader make(const ResourceDescriptor& resourceDescriptor);
        static Shader make(const void* code, size_t codeSize, ShaderType type);
        static Shader make(const std::string& filename, ShaderType type);
        void destroy() { handle_.free(); }

    public:
        [[nodiscard]] VkShaderModule ptr() const { return handle_.ptr(); }
        [[nodiscard]] ShaderType type() const;
        operator VkShaderModule() const { return handle_.ptr(); }

    private:
        Reference<VkShaderModule> handle_;
        ShaderType type_{ShaderType::Unknown};
};

template <> void Reference<VkShaderModule>::destroy();

class ShaderRegistry {
    public:
        void registerShader(const ShaderDescriptor& shaderDescriptor);
        const std::vector<ShaderDescriptor>& descriptors() { return descriptors_; };

    private:
        std::vector<ShaderDescriptor> descriptors_;
};

extern ShaderRegistry globalShaderRegistry;

///////////////////////////////////////////////////////////////////////////////
// Device Memory
///////////////////////////////////////////////////////////////////////////////

class DeviceMemory {

    public:
        enum Flags {
            None = 0x0,
            DeviceLocalMemory = 0x1,
            HostCoherentMemory = 0x2,
            HostVisibleMemory = 0x4
        };

    public:
        static DeviceMemory make(size_t size, uint32_t typeFilter, uint32_t flags);
        void destroy() { handle_.free(); }

    public:
        [[nodiscard]] VkDeviceMemory ptr() const { return handle_.ptr(); }
        operator VkDeviceMemory() const { return handle_.ptr(); }

        [[nodiscard]] size_t size() const { return size_; }
        [[nodiscard]] uint32_t flags() const { return flags_; }
        [[nodiscard]] uint32_t typeFilter() const { return typeFilter_; }

    public:
        [[nodiscard]] void* map(size_t ofs, size_t len) const;
        void unmap() const;

    private:
        Reference<VkDeviceMemory> handle_;
        size_t size_{0};
        uint32_t flags_{None};
        uint32_t typeFilter_{0};

};

template <> void Reference<VkDeviceMemory>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Descriptor Pool
///////////////////////////////////////////////////////////////////////////////

class DescriptorPool {
    public:
        static DescriptorPool make(size_t size);
        void destroy() { handle_.free(); }

    public:
        [[nodiscard]] VkDescriptorPool ptr() const { return handle_.ptr(); }
        [[nodiscard]] size_t size() const { return size_; }
        operator VkDescriptorPool() const { return handle_.ptr(); }

    private:
        Reference<VkDescriptorPool> handle_;
        size_t size_{0};
};

template <> void Reference<VkDescriptorPool>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Descriptor Set
///////////////////////////////////////////////////////////////////////////////

class DescriptorSet {
    public:
        static DescriptorSet make(VkDescriptorSetLayout layout, const DescriptorPool& descriptorPool);
        void destroy() { handle_.free(); }

    public:
        [[nodiscard]] VkDescriptorSet ptr() const { return handle_.ptr(); }
        [[nodiscard]] const VkDescriptorSet* ref_ptr() const { return handle_.ref_ptr(); }
        operator VkDescriptorSet() const { return handle_.ptr(); }

    private:
        Reference<VkDescriptorSet> handle_;
};

template <> void Reference<VkDescriptorSet>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Image
///////////////////////////////////////////////////////////////////////////////

enum class ImageType {
    Unknown = 0x0,
    PixelBuffer = 0x1,
    DepthBuffer = 0x2
};

class Image {

    public:
        static Image make(const ResourceDescriptor& resourceDescriptor);
        static Image make(const std::string& filename);
        static Image make(ImageType imageType, int width, int height, VkFormat format);
        static Image attach(VkImage image, ImageType imageType, VkFormat format);

        void destroy() {
            memory_.destroy();
            handle_.free();
        }

    private:
        void createImage(const void* pixels, int width, int height, int channels, VkFormat format);
        void createImage(ImageType imageType, int width, int height, VkFormat format);
        void transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout);
        void copyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height);

    public:
        [[nodiscard]] VkImage ptr() const { return handle_.ptr(); }
        [[nodiscard]] int width() const { return width_; }
        [[nodiscard]] int height() const { return height_; }
        [[nodiscard]] int channels() const { return channels_; }
        [[nodiscard]] size_t size() const { return size_; }
        [[nodiscard]] VkFormat format() const { return format_; }
        [[nodiscard]] ImageType imageType() const { return imageType_; }
        operator VkImage() const { return handle_.ptr(); }

    private:
        Reference<VkImage> handle_;
        int width_{0};
        int height_{0};
        int channels_{0};
        size_t size_{0};
        ImageType imageType_{ImageType::Unknown};
        VkFormat format_{VK_FORMAT_UNDEFINED};
        DeviceMemory memory_;
};

template <> void Reference<VkImage>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Image View
///////////////////////////////////////////////////////////////////////////////

class ImageView {

    public:
        static ImageView make(const Image& image);
        void destroy() { handle_.free(); }

    public:
        [[nodiscard]] VkImageView ptr() const { return handle_.ptr(); }
        [[nodiscard]] VkImage image() const { return image_; }
        [[nodiscard]] ImageType imageType() const { return imageType_; }
        [[nodiscard]] VkFormat format() const { return format_; }
        operator VkImageView() const { return handle_.ptr(); }

    private:
        Reference<VkImageView> handle_;
        VkImage image_{nullptr};
        ImageType imageType_{ImageType::Unknown};
        VkFormat format_{VK_FORMAT_UNDEFINED};
};

template <> void Reference<VkImageView>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Framebuffer
///////////////////////////////////////////////////////////////////////////////

class Framebuffer {

    public:
        static Framebuffer make(VkRenderPass renderPass, VkImageView imageView, VkImageView depthImageView, int width, int height);
        void destroy() { handle_.free(); }

    public:
        [[nodiscard]] VkFramebuffer ptr() const { return handle_.ptr(); }
        [[nodiscard]] VkRenderPass renderPass() const { return renderPass_; }
        [[nodiscard]] VkImageView image() const { return imageView_; }
        [[nodiscard]] int width() const { return width_; }
        [[nodiscard]] int height() const { return height_; }
        operator VkFramebuffer() const { return handle_.ptr(); }

    private:
        Reference<VkFramebuffer> handle_;
        VkRenderPass renderPass_{nullptr};
        VkImageView imageView_{nullptr};
        int width_{0};
        int height_{0};
};

template <> void Reference<VkFramebuffer>::destroy();

///////////////////////////////////////////////////////////////////////////////
// Sampler
///////////////////////////////////////////////////////////////////////////////

class Sampler {
    public:
        static Sampler make();
        void destroy() { handle_.free(); }

    public:
        [[nodiscard]] VkSampler ptr() const { return handle_.ptr(); }
        operator VkSampler() const { return handle_.ptr(); }

    private:
        Reference<VkSampler> handle_;

};

///////////////////////////////////////////////////////////////////////////////
// Others
///////////////////////////////////////////////////////////////////////////////

template <> void Reference<VkImage>::destroy();
template <> void Reference<VkDevice>::destroy();
template <> void Reference<VkPipeline>::destroy();
template <> void Reference<VkPipelineLayout>::destroy();
template <> void Reference<VkDescriptorSetLayout>::destroy();
template <> void Reference<VkInstance>::destroy();
template <> void Reference<VkSurfaceKHR>::destroy();
template <> void Reference<VkRenderPass>::destroy();
template <> void Reference<VkCommandPool>::destroy();
template <> void Reference<VkSampler>::destroy();

} // namespace
