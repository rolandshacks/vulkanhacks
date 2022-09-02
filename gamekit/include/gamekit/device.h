/*
 * Device
 */
#pragma once

#include "gamekit/metrics.h"
#include "gamekit/window.h"
#include "gamekit/types.h"
#include "gamekit/buffer.h"
#include "gamekit/texture.h"
#include "gamekit/frame.h"
#include "gamekit/material.h"

#include <vulkan>

#include <string>
#include <vector>
#include <functional>

namespace gamekit {

class Device {

    private:
        struct PhysicalDeviceInfo {
            int graphicsFamilyIndex{0};
            int presentFamilyIndex{0};
            bool mailBoxBodeSupport{false};
            VkSurfaceFormatKHR surfaceFormat{};
        };

        Reference<VkQueue> graphicsQueue_;
        Reference<VkQueue> presentQueue_;

        struct SwapChainInfo {
            VkSwapchainKHR handle{nullptr};
            std::vector<Image> images;
            VkFormat format{};
            VkExtent2D extent{};
            std::vector<ImageView> imageViews;

            Image depthImage;
            ImageView depthImageView;
        };

    private:
        static Device* global_instance_;

    public:
        Device();
        virtual ~Device();

    public:
        static Device* globalInstance();
        static VkDevice globalHandle();

    public:
        void createDevice(Window& window, bool enableErrorChecking);
        void destroyDevice();
        void createRenderer();
        void destroyRenderer(bool freePipelineResources=true);
        void waitIdle() const;

    public:
        void addMaterial(Material& material);
        void setMaterial(Material* material);
        Material* material() { return material_; }

    public:
        void drawIndexed(size_t count, size_t offset=0);
        void draw(size_t count, size_t offset=0, size_t instances=1);


    private:
        void reinitRenderer();

    private:
        void createInstance(Window& window);
        void destroyInstance();

        void createSurface(Window& window);
        void destroySurface();

        void createPhysicalDevice();
        void destroyPhysicalDevice();

        void createLogicalDevice();
        void destroyLogicalDevice();

        void createGraphicsPipeline();
        void freeGraphicsPipelineObjects();

        void createSwapChain();
        void destroySwapChain();

        void createImageViews();
        void destroyImageViews();

        void createRenderPass();
        void destroyRenderPass();

        void createDepthBuffer();
        void destroyDepthBuffer();

        void createFrameBuffers();
        void destroyFrameBuffers();

        void createCommandPool();
        void destroyCommandPool();

        void createFrames();
        void destroyFrames();

        void getViewportExtent();

    private:
        void beginDraw();
        void endDraw();

    public:
        bool begin(Window& window);
        bool end();
        bool isVisible() const { return visible_; }

    public:
        VkCommandBuffer beginCommand();
        void endCommand(VkCommandBuffer commandBuffer);

    public: // access methods
        VkInstance instance() const { return instance_.ptr(); }
        VkDevice handle() const { return device_.ptr(); }
        size_t frameCount() const { return MAX_FRAMES; }
        const Frame& currentFrame() const;
        VkCommandPool commandPool() const { return commandPool_.ptr(); }
        VkPhysicalDevice physicalDevice() const { return physicalDevice_.ptr(); }
        VkQueue graphicsQueue() const { return graphicsQueue_.ptr(); }
        VkRenderPass renderPass() const { return renderPass_.ptr(); }
        const Metrics& metrics() const { return metrics_; }

    private: // common
        bool enableErrorChecking_{false};

    private: // Vulkan objects
        Reference<VkInstance> instance_;
        Reference<VkPhysicalDevice> physicalDevice_;
        Reference<VkDevice> device_;
        Reference<VkSurfaceKHR> surface_;
        Reference<VkRenderPass> renderPass_;
        Reference<VkCommandPool> commandPool_;

    private:
        PhysicalDeviceInfo physicalDeviceInfo_{};
        SwapChainInfo swapChainInfo_{};

    private: // renderer
        const size_t MAX_FRAMES = 2;
        uint32_t currentImageIndex_{0};
        uint32_t currentFrame_{0};
        std::vector<Framebuffer> frameBuffers_;
        std::vector<Frame> frames_;
        Window::WindowState windowState_{0,0,false};
        bool visible_{false};
        Metrics metrics_;

    private:
        Material* material_{nullptr};
        std::vector<Material*> materials_;

    private:
        std::vector<const char*> requiredDeviceExtensions_;

};

} // namespace
