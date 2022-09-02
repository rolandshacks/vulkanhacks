/*
 * Material
 */

#include <vulkan>

#include "gamekit/material.h"
#include "gamekit/device.h"
#include "gamekit/vertex.h"
#include "gamekit/loader.h"
#include "gamekit/utilities.h"

#include <string>
#include <stdexcept>
#include <cassert>

using namespace gamekit;

Material Material::make() {

    Material material;

    material.create();

    return material;

}

void Material::create() {
    modified_ = true;
}

void Material::destroy() {
    freeDescriptorSets();
    freeGraphicsPipeline();

    buffers_.clear();
    textures_.clear();
    shaders_.clear();

    shaderStages_.clear();
    pushConstantRanges_.clear();

    numVertexBuffers_ = 0;
    numUniformBuffers_ = 0;
}

const Shader* Material::addShader(const Shader& shader) {

    shaders_.emplace_back(&shader);

    VkShaderStageFlagBits stage = (shader.type() == ShaderType::FragmentShader) ? VK_SHADER_STAGE_FRAGMENT_BIT : VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo{};
    shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.stage = stage;
    shaderStageCreateInfo.module = shader;
    shaderStageCreateInfo.pName = "main";

    shaderStages_.emplace_back(shaderStageCreateInfo);

    return &shader;
}

const Buffer* Material::addBuffer(Buffer& buffer) {
    buffers_.emplace_back(&buffer);

    switch (buffer.bufferType()) {
        case BufferType::VertexBuffer: numVertexBuffers_++; break;
        case BufferType::UniformBuffer: numUniformBuffers_++; break;
        default: break;
    }

    return &buffer;
}

const Texture* Material::addTexture(const Texture& texture, uint32_t binding) {
    textures_.emplace_back(TextureInfo{&texture, binding});
    return &texture;
}

const Texture* Material::getTexture(uint32_t binding) {
    for (const auto& textureInfo : textures_) {
        if (textureInfo.binding == binding) return textureInfo.texture;
    }
    return nullptr;
}

const PushConstantsBase* Material::addPushConstants(PushConstantsBase& pushConstants) {

    pushConstants.attachToMaterial(this);

    VkPushConstantRange range{};
    range.offset = 0;
    range.size = static_cast<uint32_t>(pushConstants.size());
    range.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;

    pushConstantRanges_.push_back(range);

    return &pushConstants;
}

void Material::updatePushConstants(const PushConstantsBase& pushConstants) {

    const auto& frame = Device::globalInstance()->currentFrame();

    const auto& commandBuffer = frame.commandBuffer;
    assert(nullptr != commandBuffer.ptr());

    vkCmdPushConstants(commandBuffer.ptr(),
                       pipelineLayout_,
                       VK_SHADER_STAGE_ALL_GRAPHICS,
                       0,
                       static_cast<uint32_t>(pushConstants.size()),
                       pushConstants.raw_ptr());


}

void Material::createGraphicsPipeline() {

    auto device = Device::globalInstance();
    assert(device);

    VkResult res = VK_SUCCESS;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // ignored because of dynamic state
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;  // ignored because of dynamic state

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto vertexBindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &vertexBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = backfaceCulling_ ? VK_CULL_MODE_BACK_BIT : VK_CULL_MODE_NONE;
    rasterizer.frontFace = fontfaceClockWise_ ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = depthTesting_ ? VK_TRUE : VK_FALSE;
    depthStencil.depthWriteEnable = depthWriting_ ? VK_TRUE : VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    auto srcBlendFactor = VK_BLEND_FACTOR_ONE;
    auto dstBlendFactor = VK_BLEND_FACTOR_ONE;

    switch (blendMode_) {
        case BlendMode::Normal: {
            srcBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            dstBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            break;
        }
        case BlendMode::Additive: {
            srcBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
            dstBlendFactor = VK_BLEND_FACTOR_ONE;
            break;
        }
        case BlendMode::Multiply: {
            srcBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
            dstBlendFactor = VK_BLEND_FACTOR_ZERO;
            break;
        }
        default: {
            break;
        }
    }

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = srcBlendFactor;
    colorBlendAttachment.dstColorBlendFactor = dstBlendFactor;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    ///////////////////////////////////////////////////////////////////////////////
    // Dynamic state changes at draw time
    ///////////////////////////////////////////////////////////////////////////////

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
        VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    ///////////////////////////////////////////////////////////////////////////////
    // Pipeline Layout
    ///////////////////////////////////////////////////////////////////////////////

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // Buffer bindings
    for (auto buffer : buffers_) {
        if (buffer->bufferType() != BufferType::UniformBuffer) continue;

        VkDescriptorSetLayoutBinding binding{};
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.binding = buffer->binding();
        binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        binding.descriptorCount = 1;
        binding.pImmutableSamplers = nullptr;

        bindings.emplace_back(binding);
    }

    // Texture bindings
    for (const auto& textureInfo : textures_) {
        VkDescriptorSetLayoutBinding binding{};
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.binding = textureInfo.binding;
        binding.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        binding.descriptorCount = 1;
        binding.pImmutableSamplers = nullptr;

        bindings.emplace_back(binding);
    }

    // Create descriptor set layout
    if (bindings.size() > 0) {
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        res = vkCreateDescriptorSetLayout(device->handle(), &layoutInfo, nullptr, descriptorSetLayout_.ref_ptr());
        if (VK_SUCCESS != res) {
            throw std::runtime_error(Format::str("Failed to create descriptor set layout: err={}", (int) res));
        }
    }

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (nullptr != descriptorSetLayout_) {
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = descriptorSetLayout_.ref_ptr();
    }

    pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges_.size());
    pipelineLayoutInfo.pPushConstantRanges = nullptr;
    if (pushConstantRanges_.size() > 0) {
        pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges_.data();
    }

    res = vkCreatePipelineLayout(device->handle(), &pipelineLayoutInfo, nullptr, pipelineLayout_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create pipeline layout: err={}", (int) res));
    }

    ///////////////////////////////////////////////////////////////////////////////
    // Pipeline
    ///////////////////////////////////////////////////////////////////////////////

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = (uint32_t) shaderStages_.size();
    pipelineInfo.pStages = shaderStages_.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout_;
    pipelineInfo.renderPass = device->renderPass();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    res = vkCreateGraphicsPipelines(device->handle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, graphicsPipeline_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create graphics pipeline: err={}", (int) res));
    }

}

void Material::freeGraphicsPipeline() {
    if (nullptr == graphicsPipeline_) return;
    graphicsPipeline_ = nullptr;
    pipelineLayout_ = nullptr;
    descriptorSetLayout_ = nullptr;
}

void Material::createDescriptorSets() {

    auto device = Device::globalInstance();

    auto numFrames = device->frameCount();
    auto numTextures = textures_.size();
    auto numDescriptors = numFrames * (numUniformBuffers_ + numTextures);

    descriptorPool_ = DescriptorPool::make(numDescriptors);

    descriptorSets_.clear();

    for (size_t frameIndex = 0; frameIndex < numFrames; frameIndex++) {
        descriptorSets_.emplace_back(createDescriptorSet());
    }

}

DescriptorSet Material::createDescriptorSet() {

    auto descriptorSet = DescriptorSet::make(descriptorSetLayout_, descriptorPool_);

    auto device = Device::globalInstance();
    auto numTextures = textures_.size();

    std::vector<VkDescriptorBufferInfo> bufferInfos;
    bufferInfos.reserve(numUniformBuffers_);
    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(numTextures);
    std::vector<VkWriteDescriptorSet> descriptorWrites;
    descriptorWrites.reserve(numUniformBuffers_ + numTextures);

    // shader buffers
    for (auto buffer : buffers_) {

        if (buffer->bufferType() != BufferType::UniformBuffer) continue;
        auto uniformBuffer = dynamic_cast<UniformBuffer*>(buffer);

        // allocate per-frame buffer
        auto& bufferObject = uniformBuffer->allocFrameBuffer();

        auto& bufferInfo = bufferInfos.emplace_back();
        bufferInfo.buffer = bufferObject;
        bufferInfo.offset = 0;
        bufferInfo.range = bufferObject.size();

        auto& descriptorWrite = descriptorWrites.emplace_back();
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = buffer->binding();
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
    }

    // textures
    for (const auto& textureInfo : textures_) {
        auto texture = textureInfo.texture;

        auto& imageInfo = imageInfos.emplace_back();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = texture->imageView();
        imageInfo.sampler = texture->sampler();

        auto& descriptorWrite = descriptorWrites.emplace_back();
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = textureInfo.binding;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
    }

    // update descriptor sets
    if (descriptorWrites.size() > 0) {
        vkUpdateDescriptorSets(
            device->handle(),
            static_cast<uint32_t>(descriptorWrites.size()),
            descriptorWrites.data(),
            0, nullptr);
    }

    return descriptorSet;
}

void Material::freeDescriptorSets() {
    descriptorPool_.destroy();
    descriptorSets_.clear();
}

void Material::compile() {
    update();
}

void Material::update() {
    if (false == modified_) return;

    freeDescriptorSets();
    freeGraphicsPipeline();

    createGraphicsPipeline();
    createDescriptorSets();

    modified_ = false;
}

void Material::bind() {

    update();

    assert(nullptr != graphicsPipeline_);

    const auto& frame = Device::globalInstance()->currentFrame();
    const auto& commandBuffer = frame.commandBuffer;

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

    setDynamicStates();

    const auto& descriptorSet = descriptorSets_[frame.index];

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout_,
                            0, 1, descriptorSet.ref_ptr(),
                            0, nullptr);
}

void Material::setDynamicStates() {

    const auto& frame = Device::globalInstance()->currentFrame();
    const auto& commandBuffer = frame.commandBuffer;

    vkCmdSetDepthTestEnableEXT(commandBuffer, depthTesting_ ? VK_TRUE : VK_FALSE);
    vkCmdSetDepthWriteEnableEXT(commandBuffer, depthWriting_ ? VK_TRUE : VK_FALSE);
}
