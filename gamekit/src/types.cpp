/*
 * Utilities
 */

#include <vulkan>

#include "gamekit/device.h"
#include "gamekit/types.h"
#include "gamekit/utilities.h"

#include <fstream>
#include <stdexcept>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace gamekit;

///////////////////////////////////////////////////////////////////////////////
// Semaphore
///////////////////////////////////////////////////////////////////////////////

Semaphore Semaphore::make() {
    auto device = Device::globalHandle();
    assert(nullptr != device);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    Semaphore object;

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, object.handle_.ref_ptr()) != VK_SUCCESS) {
        throw std::runtime_error("failed to create semaphore");
    }

    return object;
}

template <> void Reference<VkSemaphore>::destroy() {
     auto device = Device::globalHandle();
    vkDestroySemaphore(device, handle_, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// Fence
///////////////////////////////////////////////////////////////////////////////

Fence Fence::make(bool signaled) {
    auto device = Device::globalHandle();
    assert(nullptr != device);

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    if (signaled) fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    Fence object;

    if (vkCreateFence(device, &fenceInfo, nullptr, object.handle_.ref_ptr()) != VK_SUCCESS) {
        throw std::runtime_error("failed to create fence");
    }

    return object;
}

template <> void Reference<VkFence>::destroy() {
     auto device = Device::globalHandle();
    vkDestroyFence(device, handle_, nullptr);
}

VkResult Fence::wait(nanosecond_t timeout) const {
    auto device = Device::globalHandle();
    uint64_t t = (timeout >= 0) ? ((uint64_t) timeout) : UINT64_MAX;
    return vkWaitForFences(device, 1, handle_.ref_ptr(), VK_TRUE, t);
}

VkResult Fence::waitAndReset(nanosecond_t timeout) const {
    auto result = wait(timeout);
    if (VK_SUCCESS != result) return result;
    return reset();
}

VkResult Fence::reset() const {
    auto device = Device::globalHandle();
    return vkResetFences(device, 1, handle_.ref_ptr());
}

///////////////////////////////////////////////////////////////////////////////
// Command Buffer
///////////////////////////////////////////////////////////////////////////////

CommandBuffer CommandBuffer::make() {
    auto device = Device::globalHandle();
    assert(nullptr != device);

    auto commandPool = Device::globalInstance()->commandPool();
    assert(nullptr != commandPool);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    CommandBuffer object;

    auto res = vkAllocateCommandBuffers(device, &allocInfo, object.handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to allocate command buffer: err={}", (int) res));
    }

    return object;
}

template <> void Reference<VkCommandBuffer>::destroy() {
    auto device = Device::globalHandle();
    auto commandPool = Device::globalInstance()->commandPool();
    vkFreeCommandBuffers(device, commandPool, 1, &handle_);
}

VkResult CommandBuffer::reset() {
    return vkResetCommandBuffer(handle_.ptr(), 0);
}

VkResult CommandBuffer::begin() {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    return vkBeginCommandBuffer(handle_.ptr(), &beginInfo);
}

VkResult CommandBuffer::end() {
    return vkEndCommandBuffer(handle_.ptr());
}

///////////////////////////////////////////////////////////////////////////////
// Shader
///////////////////////////////////////////////////////////////////////////////

Shader Shader::make(const ResourceDescriptor& resourceDescriptor) {

    ShaderType shaderType = ShaderType::Unknown;
    if (resourceDescriptor.type == ResourceType::VertexShader) {
        shaderType = ShaderType::VertexShader;
    } else if (resourceDescriptor.type == ResourceType::FragmentShader) {
        shaderType = ShaderType::FragmentShader;
    } else {
        throw std::runtime_error("unsupported resource type for shader");
    }

    return make(resourceDescriptor.data, resourceDescriptor.dataSize, shaderType);
}

Shader Shader::make(const void* code, size_t codeSize, ShaderType type) {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = codeSize;
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code);

    Shader object;

    object.type_ = type;

    auto res = vkCreateShaderModule(device, &createInfo, nullptr, object.handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error("failed to create shader");
    }

    return object;
}


Shader Shader::make(const std::string& filename, ShaderType type) {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to load shader file");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), (std::streamsize) fileSize);
    file.close();

    return make(buffer.data(), buffer.size(), type);
}

template <> void Reference<VkShaderModule>::destroy() {
    auto device = Device::globalHandle();
    vkDestroyShaderModule(device, handle_, nullptr);
}

ShaderType Shader::type() const {
    return type_;
}

void ShaderRegistry::registerShader(const ShaderDescriptor& shaderDescriptor) {
    descriptors_.emplace_back(shaderDescriptor);
}

///////////////////////////////////////////////////////////////////////////////
// Device Memory
///////////////////////////////////////////////////////////////////////////////

DeviceMemory DeviceMemory::make(size_t size, uint32_t typeFilter, uint32_t flags) {

    VkMemoryPropertyFlags propertyFlags = 0x0;
    if (0x0 != (flags & Flags::DeviceLocalMemory))  propertyFlags |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (0x0 != (flags & Flags::HostCoherentMemory)) propertyFlags |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (0x0 != (flags & Flags::HostVisibleMemory))  propertyFlags |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    auto physicalDevice = Device::globalInstance()->physicalDevice();
    assert(nullptr != physicalDevice);

    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    uint32_t typeIndex = 0;
    bool found = false;

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
         if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            typeIndex = i;
            found = true;
            break;
        }
    }

    if (!found) {
        throw std::runtime_error("failed to find suitable memory type!");
    }

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = size;
    allocInfo.memoryTypeIndex = typeIndex;

    auto device = Device::globalHandle();

    DeviceMemory object;

    object.size_ = size;
    object.typeFilter_ = typeFilter;
    object.flags_ = flags;

    auto res = vkAllocateMemory(device, &allocInfo, nullptr, object.handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to allocate vertex buffer memory: err={}", (int) res));
    }

    return object;
}

template <> void Reference<VkDeviceMemory>::destroy() {
    auto device = Device::globalHandle();
    vkFreeMemory(device, handle_, nullptr);
}

void* DeviceMemory::map(size_t ofs, size_t len) const {
    auto device = Device::globalHandle();
    void* ptr{nullptr};
    vkMapMemory(device, handle_.ptr(), ofs, len, 0, &ptr);
    return ptr;
}

void DeviceMemory::unmap() const {
    auto device = Device::globalHandle();
    vkUnmapMemory(device, handle_.ptr());
}

///////////////////////////////////////////////////////////////////////////////
// Descriptor Pool
///////////////////////////////////////////////////////////////////////////////

DescriptorPool DescriptorPool::make(size_t size) {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>(size);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(size);

    DescriptorPool object;

    object.size_ = size;

    auto res = vkCreateDescriptorPool(device, &poolInfo, nullptr, object.handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create descriptor pool: err={}", (int) res));
    }

    return object;
}

template <> void Reference<VkDescriptorPool>::destroy() {
    auto device = Device::globalHandle();
    vkDestroyDescriptorPool(device, handle_, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// Descriptor Set
///////////////////////////////////////////////////////////////////////////////

DescriptorSet DescriptorSet::make(VkDescriptorSetLayout layout, const DescriptorPool& descriptorPool) {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    DescriptorSet object;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool.ptr();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    auto res = vkAllocateDescriptorSets(device, &allocInfo, object.handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to allocate descriptor set: err={}", (int) res));
    }

    return object;
}

template <> void Reference<VkDescriptorSet>::destroy() {}

///////////////////////////////////////////////////////////////////////////////
// Image
///////////////////////////////////////////////////////////////////////////////

Image Image::make(const ResourceDescriptor& resourceDescriptor) {

    int imageWidth = 0;
    int imageHeight = 0;
    int imageChannels = 0;

    auto pixels = stbi_load_from_memory(static_cast<const stbi_uc*>(resourceDescriptor.data), (int) resourceDescriptor.dataSize, &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);
    if (nullptr == pixels) {
        throw std::runtime_error("failed to load image from file!");
    }

    Image object;
    object.createImage(pixels, imageWidth, imageHeight, imageChannels, VK_FORMAT_R8G8B8A8_SRGB);
    stbi_image_free(pixels);
    pixels = nullptr;

    return object;
}

Image Image::make(const std::string& filename) {

    int imageWidth = 0;
    int imageHeight = 0;
    int imageChannels = 0;

    auto pixels = stbi_load(filename.c_str(), &imageWidth, &imageHeight, &imageChannels, STBI_rgb_alpha);
    if (nullptr == pixels) {
        throw std::runtime_error("failed to load image from file!");
    }

    Image object;
    object.createImage(pixels, imageWidth, imageHeight, imageChannels, VK_FORMAT_R8G8B8A8_SRGB);
    stbi_image_free(pixels);
    pixels = nullptr;

    return object;
}

Image Image::make(ImageType imageType, int width, int height, VkFormat format) {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    Image object;

    object.width_ = width;
    object.height_ = height;
    object.channels_ = 0;
    object.size_ = 0;
    object.format_ = format;
    object.imageType_ = imageType;

    object.createImage(imageType, width, height, format);

    return object;
}

Image Image::attach(VkImage image, ImageType imageType, VkFormat format) {

    Image object;

    object.handle_.attach(image);
    object.width_ = 0;
    object.height_ = 0;
    object.channels_ = 0;
    object.size_ = 0;
    object.format_ = format;
    object.imageType_ = imageType;

    return object;
}

void Image::createImage(const void* pixels, int width, int height, int channels, VkFormat format) {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    auto imageSize = static_cast<size_t>(width * height * 4);

    auto stagingBuffer = BufferObject::make(
        BufferType::StagingBuffer,
        imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        DeviceMemory::HostVisibleMemory | DeviceMemory::HostCoherentMemory);

    stagingBuffer.copy(pixels);

    createImage(ImageType::PixelBuffer, width, height, format);
    transitionImageLayout(VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer.ptr(), static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    transitionImageLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    stagingBuffer.destroy();

}

void Image::createImage(ImageType imageType, int width, int height, VkFormat format) {

    imageType_ = imageType;
    width_ = width;
    height_ = height;
    format_ = format;
    channels_ = 4;
    size_ = static_cast<size_t>(width * height * 4);

    auto device = Device::globalHandle();
    assert(nullptr != device);

    uint32_t usageFlags = 0x0;

    if (ImageType::DepthBuffer == imageType) {
        usageFlags = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    } else {
        usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usageFlags;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    auto res = vkCreateImage(device, &imageInfo, nullptr, handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create image: err={}", (int) res));
    }

    VkMemoryRequirements memRequirements{};
    vkGetImageMemoryRequirements(device, handle_.ptr(), &memRequirements);

    memory_ = DeviceMemory::make(memRequirements.size,
                                 memRequirements.memoryTypeBits,
                                 DeviceMemory::DeviceLocalMemory);

    res = vkBindImageMemory(device, handle_.ptr(), memory_.ptr(), 0);
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to bind image memory: err={}", (int) res));
    }
}

void Image::transitionImageLayout(VkImageLayout oldLayout, VkImageLayout newLayout) {

    auto device = Device::globalInstance();
    assert(nullptr != device);

    VkCommandBuffer commandBuffer = device->beginCommand();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = handle_.ptr();
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    device->endCommand(commandBuffer);
}

void Image::copyBufferToImage(VkBuffer buffer, uint32_t width, uint32_t height) {

    auto device = Device::globalInstance();
    assert(nullptr != device);

    VkCommandBuffer commandBuffer = device->beginCommand();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, handle_.ptr(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    device->endCommand(commandBuffer);
}

///////////////////////////////////////////////////////////////////////////////
// Image View
///////////////////////////////////////////////////////////////////////////////

ImageView ImageView::make(const Image& image) {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    auto imageType = image.imageType();

    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_NONE;
    if (ImageType::DepthBuffer == imageType) {
        aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    } else {
        aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = image.ptr();
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = image.format();
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = aspectMask;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    ImageView object;

    object.image_ = image.ptr();
    object.imageType_ = image.imageType();
    object.format_ = image.format();

    auto res = vkCreateImageView(device, &createInfo, nullptr, object.handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("failed to create image view: err={}", (int) res));
    }

    return object;
}

template <> void Reference<VkImageView>::destroy() {
    auto device = Device::globalHandle();
    vkDestroyImageView(device, handle_, nullptr);
}


///////////////////////////////////////////////////////////////////////////////
// Framebuffer
///////////////////////////////////////////////////////////////////////////////

Framebuffer Framebuffer::make(VkRenderPass renderPass, VkImageView imageView, VkImageView depthImageView, int width, int height) {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    std::array<VkImageView, 2> attachments = {
        imageView,
        depthImageView
    };

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = static_cast<uint32_t>(width);
    framebufferInfo.height = static_cast<uint32_t>(height);
    framebufferInfo.layers = 1;

    Framebuffer object;

    object.renderPass_ = renderPass;
    object.imageView_ = imageView;
    object.width_ = width;
    object.height_ = height;

    auto res = vkCreateFramebuffer(device, &framebufferInfo, nullptr, object.handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create framebuffer: err={}", (int) res));
    }

    return object;
}

template <> void Reference<VkFramebuffer>::destroy() {
    auto device = Device::globalHandle();
    vkDestroyFramebuffer(device, handle_, nullptr);
}


///////////////////////////////////////////////////////////////////////////////
// Sampler
///////////////////////////////////////////////////////////////////////////////

Sampler Sampler::make() {

    auto device = Device::globalHandle();
    assert(nullptr != device);

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(Device::globalInstance()->physicalDevice(), &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    Sampler object;

    auto res = vkCreateSampler(device, &samplerInfo, nullptr, object.handle_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create sampler: err={}", (int) res));
    }

    return object;
}

template <> void Reference<VkSampler>::destroy() {
    auto device = Device::globalHandle();
    vkDestroySampler(device, handle_, nullptr);
}

///////////////////////////////////////////////////////////////////////////////
// Others
///////////////////////////////////////////////////////////////////////////////

template <> void Reference<VkDevice>::destroy() {
    vkDeviceWaitIdle(handle_);
    vkDestroyDevice(handle_, nullptr);
}

template <> void Reference<VkPipeline>::destroy() {
    vkDestroyPipeline(Device::globalHandle(), handle_, nullptr);
}

template <> void Reference<VkPipelineLayout>::destroy() {
    vkDestroyPipelineLayout(Device::globalHandle(), handle_, nullptr);
}

template <> void Reference<VkDescriptorSetLayout>::destroy() {
    vkDestroyDescriptorSetLayout(Device::globalHandle(), handle_, nullptr);
}

template <> void Reference<VkInstance>::destroy() {
    vkDestroyInstance(handle_, nullptr);
}

template <> void Reference<VkSurfaceKHR>::destroy() {
    vkDestroySurfaceKHR(Device::globalInstance()->instance(), handle_, nullptr);
}

template <> void Reference<VkRenderPass>::destroy() {
    auto device = Device::globalHandle();
    if (nullptr != device) {
        vkDeviceWaitIdle(device);
        vkDestroyRenderPass(device, handle_, nullptr);
    }
}

template <> void Reference<VkCommandPool>::destroy() {
    vkDestroyCommandPool(Device::globalHandle(), handle_, nullptr);
}

template <> void Reference<VkImage>::destroy() {
    auto device = Device::globalHandle();
    vkDestroyImage(device, handle_, nullptr);
}
