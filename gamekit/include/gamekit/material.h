/*
 * Material
 */
#pragma once

#include "gamekit/types.h"
#include "gamekit/buffer.h"
#include "gamekit/texture.h"

#include <vulkan>
#include <string>
#include <vector>

namespace gamekit {

enum class BlendMode {
    Normal = 0x1,
    Additive = 0x2,
    Multiply = 0x3
};

class Material {

    public:
        static Material make();

    private:
        void create();
        void update();
        void createGraphicsPipeline();
        void freeGraphicsPipeline();
        void createDescriptorSets();
        void freeDescriptorSets();
        DescriptorSet createDescriptorSet();
        void setDynamicStates();

    public:
        void compile();
        void bind();
        void destroy();

    public:
        const Shader* addShader(const Shader& shader);
        const Buffer* addBuffer(Buffer& buffer);
        const Texture* addTexture(const Texture& texture, uint32_t binding);
        const PushConstantsBase* addPushConstants(PushConstantsBase& pushConstants);

    public:
        void updatePushConstants(const PushConstantsBase& pushConstants);
        const Texture* getTexture(uint32_t binding);

    public: // setters
        void setEnableBlending(bool enableBlending) { enableBlending_ = enableBlending; }
        void setBlendMode(BlendMode blendMode) { blendMode_ = blendMode; }
        void setBackfaceCulling(bool backfaceCulling) { backfaceCulling_ = backfaceCulling; };
        void setFontFaceClockwise(bool fontfaceClockWise) { fontfaceClockWise_ = fontfaceClockWise; };
        void setDepthTesting(bool depthTesting) { depthTesting_ = depthTesting; };
        void setDepthWriting(bool depthWriting) { depthWriting_ = depthWriting; };

    public: // getters
        bool enableBlending() const { return enableBlending_; }
        BlendMode blendMode() const { return blendMode_; }
        const DescriptorPool& descriptorPool() { return descriptorPool_; }

    private:
        bool enableBlending_{false};
        BlendMode blendMode_{BlendMode::Normal};
        bool backfaceCulling_{true};
        bool fontfaceClockWise_{false};
        bool depthTesting_{false};
        bool depthWriting_{false};

    private:
        bool modified_{false};
        DescriptorPool descriptorPool_;
        Reference<VkDescriptorSetLayout> descriptorSetLayout_;
        Reference<VkPipelineLayout> pipelineLayout_;
        Reference<VkPipeline> graphicsPipeline_;
        std::vector<DescriptorSet> descriptorSets_;

    private:
        struct TextureInfo {
            const Texture* texture{nullptr};
            uint32_t binding{0};
        };

    private:
        std::vector<Buffer*> buffers_;
        size_t numVertexBuffers_{0};
        size_t numUniformBuffers_{0};
        std::vector<TextureInfo> textures_;
        std::vector<const Shader*> shaders_;
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages_;
        std::vector<VkPushConstantRange> pushConstantRanges_;

};

} // namespace
