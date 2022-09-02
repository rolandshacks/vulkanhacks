/*
 * Device
 */

#include <vulkan>

#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <cassert>

#include "gamekit/metrics.h"
#include "gamekit/types.h"
#include "gamekit/vertex.h"
#include "gamekit/clock.h"
#include "gamekit/device.h"
#include "gamekit/loader.h"
#include "gamekit/utilities.h"

using namespace gamekit;

static const char* SEVERITY_VERBOSE = "DEBUG";
static const char* SEVERITY_INFO    = "INFO";
static const char* SEVERITY_WARNING = "WARNING";
static const char* SEVERITY_ERROR   = "ERROR";

static const bool ENABLE_EXTENDED_DYNAMIC_STATE = true;
static const std::string EXT_SWAPCHAIN = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
static const std::string EXT_DYNAMIC_STATE = VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME;

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData);


#define RESOLVE_VK_INSTANCE_PFN(instance, funcName)                                          \
    {                                                                                        \
        funcName =                                                                           \
            reinterpret_cast<PFN_##funcName>(vkGetInstanceProcAddr(instance, "" #funcName)); \
        if (funcName == nullptr) {                                                           \
            const std::string name = #funcName;                                              \
            std::cout << "Failed to resolve function " << name << std::endl;                 \
        }                                                                                    \
    }

#define RESOLVE_VK_DEVICE_PFN(device, funcName)                                                 \
    {                                                                                           \
        funcName = reinterpret_cast<PFN_##funcName>(vkGetDeviceProcAddr(device, "" #funcName)); \
        if (funcName == nullptr) {                                                              \
            const std::string name = #funcName;                                                 \
            std::cout << "Failed to resolve function " << name << std::endl;                    \
        }                                                                                       \
    }

Device* Device::global_instance_{nullptr};

Device::Device() {
    if (nullptr == global_instance_) {
        global_instance_ = this;
    }

    requiredDeviceExtensions_.emplace_back(EXT_SWAPCHAIN.c_str());
    if constexpr (ENABLE_EXTENDED_DYNAMIC_STATE) {
        requiredDeviceExtensions_.emplace_back(EXT_DYNAMIC_STATE.c_str());
    }
}

Device::~Device() {
    if (this == global_instance_) {
        global_instance_ = nullptr;
    }

    destroyRenderer();
    destroyDevice();
}

Device* Device::globalInstance() {
    return global_instance_;
}

VkDevice Device::globalHandle() {
    return global_instance_->device_;
}

void Device::createDevice(Window& window, bool enableErrorChecking) {

    enableErrorChecking_ = enableErrorChecking;

    createInstance(window);

    Loader::instance()->registerInstance(instance_);

    createSurface(window);
    createPhysicalDevice();
    createLogicalDevice();
    createCommandPool();

    visible_ = true;
}

void Device::destroyDevice() {
    visible_ = false;

    destroyCommandPool();
    destroyPhysicalDevice();
    destroyLogicalDevice();
    destroySurface();

    Loader::instance()->unregisterInstance();
    destroyInstance();
}

void Device::createRenderer() {
    assert(device_.notNull());
    createSwapChain();
    createImageViews();
    createDepthBuffer();
    createRenderPass();
    createFrameBuffers();
    createFrames();
}

void Device::destroyRenderer(bool freePipelineResources) {

    visible_ = false;

    if (device_.isNull()) {
        return;
    }

    waitIdle();

    if (freePipelineResources) {
        freeGraphicsPipelineObjects();
    }

    destroyFrames();
    destroyFrameBuffers();

    destroyRenderPass();
    destroyDepthBuffer();
    destroyImageViews();
    destroySwapChain();

}

void Device::reinitRenderer() {
    assert(device_.notNull());

    waitIdle();

    if (visible_) {
        visible_ = false;
        destroyDepthBuffer();
        destroyFrameBuffers();
        destroyImageViews();
        destroySwapChain();
    }

    if (windowState_.minimized) {
        return; // no renderer setup when minimized
    }

    createSwapChain();
    createImageViews();
    createDepthBuffer();
    createFrameBuffers();

    visible_ = true;
}

void Device::createInstance(Window& window) {

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VDemo";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;
    instanceInfo.pApplicationInfo = &appInfo;

    auto requiredInstanceExtensions = window.getVulkanExtensions();

    if constexpr (ENABLE_EXTENDED_DYNAMIC_STATE) {
        requiredInstanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }

    const std::vector<const char*> requiredValidationLayers = { "VK_LAYER_KHRONOS_validation" };

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableErrorChecking_) {

        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        std::set<std::string> requiredLayers(requiredValidationLayers.begin(), requiredValidationLayers.end());

        for (const auto& availableLayer : availableLayers) {
            //std::cout << "available validation layer: " << availableLayer.layerName << std::endl;
            requiredLayers.erase(availableLayer.layerName);
        }

        if (!requiredLayers.empty()) {
            // required validation layers not supported
            throw std::runtime_error("required validation layers not supported");
        }

        instanceInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());;
        instanceInfo.ppEnabledLayerNames = requiredValidationLayers.data();;

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        instanceInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;

        requiredInstanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    } else {
        instanceInfo.enabledLayerCount = 0;
        instanceInfo.ppEnabledLayerNames = nullptr;
        instanceInfo.pNext = nullptr;
    }

    instanceInfo.enabledExtensionCount = (uint32_t) requiredInstanceExtensions.size();
    instanceInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();

    auto res = vkCreateInstance(&instanceInfo, nullptr, instance_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Vulkan initialization failed: err={}", (int) res));
    }

}

void Device::destroyInstance() {
    instance_ = nullptr;
}

void Device::createSurface(Window& window) {
    window.createVulkanSurface(instance_, surface_.ref_ptr());
}

void Device::destroySurface() {
    surface_ = nullptr;
}

void Device::createPhysicalDevice() {

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);

    if (0 == deviceCount) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    // select a physical device
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    for (const auto& device : devices) {

        VkPhysicalDeviceProperties deviceProperties{};
        vkGetPhysicalDeviceProperties(device, &deviceProperties);

        if (deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
            deviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            // TODO: extend by rating and choosing best possible GPU
            continue;
        }

        // check for required device extensions
        uint32_t deviceExtensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, nullptr);

        std::vector<VkExtensionProperties> availableDeviceExtensions(deviceExtensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &deviceExtensionCount, availableDeviceExtensions.data());

        std::set<std::string> requiredExtensions(requiredDeviceExtensions_.begin(), requiredDeviceExtensions_.end());

        for (const auto& availableExtension : availableDeviceExtensions) {
            requiredExtensions.erase(availableExtension.extensionName);
        }

        if (!requiredExtensions.empty()) {
            // device does not support the required extensions
            continue;
        }

        // check surface formats
        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
        if (0 == formatCount) {
            continue;
        }

        std::vector<VkSurfaceFormatKHR> deviceSurfaceFormats(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, deviceSurfaceFormats.data());

        bool foundSwapSpaceSurfaceFormat = false;
        for (const auto& surfaceFormat : deviceSurfaceFormats) {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                foundSwapSpaceSurfaceFormat = true;
                physicalDeviceInfo_.surfaceFormat = surfaceFormat;
                break;
            }
        }

        if (!foundSwapSpaceSurfaceFormat) {
            continue;
        }

        // check present mode
        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
        if (0 == presentModeCount) {
            continue;
        }
        std::vector<VkPresentModeKHR> devicePresentModes(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, devicePresentModes.data());

        physicalDeviceInfo_.mailBoxBodeSupport = false;
        for (const auto& mode : devicePresentModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                physicalDeviceInfo_.mailBoxBodeSupport = true; // extended "triple-buffer-like" mode
                break;
            }
        }

        // check for graphics and presentation queue family support
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        physicalDeviceInfo_.graphicsFamilyIndex = physicalDeviceInfo_.presentFamilyIndex = -1;

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {

            if (-1 == physicalDeviceInfo_.graphicsFamilyIndex) {
                if (0 != (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                    physicalDeviceInfo_.graphicsFamilyIndex = i;
                }
            }

            if (-1 == physicalDeviceInfo_.presentFamilyIndex) {
                VkBool32 presentSupport = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
                if (VK_TRUE == presentSupport) {
                    physicalDeviceInfo_.presentFamilyIndex = i;
                }
            }

             if (-1 != physicalDeviceInfo_.graphicsFamilyIndex && -1 != physicalDeviceInfo_.presentFamilyIndex) {
                 break;
             }

            i++;
        }

        if (-1 == physicalDeviceInfo_.graphicsFamilyIndex || -1 == physicalDeviceInfo_.presentFamilyIndex) {
            continue;
        }

        physicalDevice = device;

        break;
    }

    if (nullptr == physicalDevice) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    physicalDevice_ = physicalDevice;

    getViewportExtent();

}

void Device::destroyPhysicalDevice() {
    physicalDevice_ = nullptr;
}

void Device::createLogicalDevice() {
    // prepare logical device create structures

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {(uint32_t) physicalDeviceInfo_.graphicsFamilyIndex,
                                              (uint32_t) physicalDeviceInfo_.presentFamilyIndex};
    float queuePriority = 1.0f;
    for (auto queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice_, &deviceFeatures);
    createInfo.pEnabledFeatures = &deviceFeatures;

    VkPhysicalDeviceFeatures2 deviceFeatures2{};
    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT dynamicStateFeatures{};
    VkPhysicalDeviceExtendedDynamicState2FeaturesEXT dynamicState2Features{};

    //if constexpr (ENABLE_EXTENDED_DYNAMIC_STATE) {

        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
        deviceFeatures2.pNext = &dynamicStateFeatures;

        dynamicStateFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT;
        dynamicStateFeatures.pNext = &dynamicState2Features;

        dynamicState2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_2_FEATURES_EXT;
        dynamicState2Features.pNext = nullptr;

        vkGetPhysicalDeviceFeatures2(physicalDevice_, &deviceFeatures2);
        assert (VK_TRUE == dynamicStateFeatures.extendedDynamicState);

        // enable extended device features
        createInfo.pEnabledFeatures = nullptr; // must be set to null
        createInfo.pNext = &deviceFeatures2;
        deviceFeatures2.pNext = &dynamicStateFeatures;
        dynamicStateFeatures.pNext = &dynamicState2Features;
        dynamicState2Features.pNext = nullptr;

    //}

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredDeviceExtensions_.size());
    createInfo.ppEnabledExtensionNames = requiredDeviceExtensions_.data();

    // create logical device
    auto res = vkCreateDevice(physicalDevice_, &createInfo, nullptr, device_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create logical device: err={}", (int) res));
    }

    // retrieve queue handles
    vkGetDeviceQueue(device_, physicalDeviceInfo_.graphicsFamilyIndex, 0, graphicsQueue_.ref_ptr());
    vkGetDeviceQueue(device_, physicalDeviceInfo_.presentFamilyIndex, 0, presentQueue_.ref_ptr());
}

void Device::destroyLogicalDevice() {
    device_ = nullptr;
}

void Device::getViewportExtent() {

    VkSurfaceCapabilitiesKHR deviceSurfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &deviceSurfaceCapabilities);

    // get swap extent
    if (deviceSurfaceCapabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        metrics_.width = deviceSurfaceCapabilities.currentExtent.width;
        metrics_.height = deviceSurfaceCapabilities.currentExtent.height;
    } else {
        auto minExtent = deviceSurfaceCapabilities.minImageExtent;
        auto maxExtent = deviceSurfaceCapabilities.maxImageExtent;
        metrics_.width = std::clamp((uint32_t) windowState_.width, minExtent.width, maxExtent.width);
        metrics_.height = std::clamp((uint32_t) windowState_.height, minExtent.height, maxExtent.height);
    }

    metrics_.width_f = (float) metrics_.width;
    metrics_.height_f = (float) metrics_.height;

}

void Device::createSwapChain() {

    getViewportExtent();
    swapChainInfo_.extent = { (uint32_t) metrics_.width, (uint32_t) metrics_.height };

    VkResult res = VK_SUCCESS;

    VkSurfaceCapabilitiesKHR deviceSurfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &deviceSurfaceCapabilities);

    // set the number of swap chain image buffers
    uint32_t imageCount = deviceSurfaceCapabilities.minImageCount + 1;
    if (deviceSurfaceCapabilities.maxImageCount > 0 && imageCount > deviceSurfaceCapabilities.maxImageCount) {
        imageCount = deviceSurfaceCapabilities.maxImageCount;
    }

    // swap buffer mode (mailbox: triple-buffer, fifo: v-sync, immediate: no v-sync, fifo relaxed: no v-sync if late)
    VkPresentModeKHR swapChainPresentMode = physicalDeviceInfo_.mailBoxBodeSupport ? VK_PRESENT_MODE_MAILBOX_KHR : VK_PRESENT_MODE_FIFO_KHR;

    // create swap chain
    VkSwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = surface_;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageFormat = physicalDeviceInfo_.surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = physicalDeviceInfo_.surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = swapChainInfo_.extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (physicalDeviceInfo_.graphicsFamilyIndex != physicalDeviceInfo_.presentFamilyIndex) {
        uint32_t queueFamilyIndices[] = {(uint32_t) physicalDeviceInfo_.graphicsFamilyIndex, (uint32_t) physicalDeviceInfo_.presentFamilyIndex};
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapChainCreateInfo.queueFamilyIndexCount = 2;
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
        swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
    }

    swapChainCreateInfo.preTransform = deviceSurfaceCapabilities.currentTransform;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapChainCreateInfo.presentMode = swapChainPresentMode;
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    res = vkCreateSwapchainKHR(device_, &swapChainCreateInfo, nullptr, &swapChainInfo_.handle);
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create swap chain: err={}", (int) res));
    }

    swapChainInfo_.format = swapChainCreateInfo.imageFormat;
}

void Device::createImageViews() {

    uint32_t swapChainImageCount = 0;
    auto res = vkGetSwapchainImagesKHR(device_, swapChainInfo_.handle, &swapChainImageCount, nullptr);
    if (0 == swapChainImageCount) {
        throw std::runtime_error(Format::str("Failed to create swap chain: err={}", (int) res));
    }

    std::vector<VkImage> imageHandles;
    imageHandles.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(device_, swapChainInfo_.handle, &swapChainImageCount, imageHandles.data());

    swapChainInfo_.images.clear();
    swapChainInfo_.images.reserve(swapChainImageCount);
    swapChainInfo_.imageViews.clear();
    for (auto handle : imageHandles) {
        auto& image = swapChainInfo_.images.emplace_back(Image::attach(handle, ImageType::PixelBuffer, physicalDeviceInfo_.surfaceFormat.format));
        swapChainInfo_.imageViews.emplace_back(ImageView::make(image));
    }
}

void Device::createRenderPass() {

    assert(device_.notNull());

    VkResult res = VK_SUCCESS;

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainInfo_.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = swapChainInfo_.depthImage.format();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    res = vkCreateRenderPass(device_, &renderPassInfo, nullptr, renderPass_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create render pass: err={}", (int) res));
    }
}

void Device::createDepthBuffer() {
    assert(device_.notNull());

    std::vector<VkFormat> supportedFormats = {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
    };

    VkFormat depthFormat = VK_FORMAT_UNDEFINED;

    for (VkFormat format : supportedFormats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice_, format, &props);
        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthFormat = format;
            break;
        }
    }

    if (VK_FORMAT_UNDEFINED == depthFormat) {
        throw std::runtime_error("Failed to find supported depth buffer format");
    }

    swapChainInfo_.depthImage = Image::make(ImageType::DepthBuffer, swapChainInfo_.extent.width, swapChainInfo_.extent.height, depthFormat);
    swapChainInfo_.depthImageView = ImageView::make(swapChainInfo_.depthImage);
}

void Device::destroyDepthBuffer() {
    assert(device_.notNull());

    swapChainInfo_.depthImageView.destroy();
    swapChainInfo_.depthImage.destroy();
}

void Device::createFrameBuffers() {

    assert(device_.notNull());

    auto width = swapChainInfo_.extent.width;
    auto height = swapChainInfo_.extent.height;

    frameBuffers_.clear();
    for (size_t i = 0; i < swapChainInfo_.imageViews.size(); i++) {
        const auto& imageView = swapChainInfo_.imageViews[i];
        frameBuffers_.emplace_back(Framebuffer::make(
            renderPass_,
            imageView,
            swapChainInfo_.depthImageView,
            width,
            height
        ));
    }

}

void Device::createFrames() {

    auto numFrames = frameCount();

    currentFrame_ = 0;
    frames_.resize(numFrames);
    uint32_t frameIndex = 0;
    for (auto& frame : frames_) {
        frame.create(frameIndex++);
    }

    for (auto material : materials_) {
        material->compile();
    }
}

void Device::addMaterial(Material& material) {
    materials_.emplace_back(&material);
    setMaterial(&material);
}

void Device::createCommandPool() {

    assert(physicalDeviceInfo_.graphicsFamilyIndex >= 0);

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // create command pool
    ///////////////////////////////////////////////////////////////////////////////////////////////

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = physicalDeviceInfo_.graphicsFamilyIndex;

    auto res = vkCreateCommandPool(device_, &poolInfo, nullptr, commandPool_.ref_ptr());
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to create command pool: err={}", (int) res));
    }

}

void Device::destroySwapChain() {

    if (device_.isNull()) return;

    if (nullptr != swapChainInfo_.handle) {
        vkDestroySwapchainKHR(device_, swapChainInfo_.handle, nullptr);
        swapChainInfo_.handle = nullptr;
    }
}

void Device::destroyImageViews() {
    if (device_.isNull()) return;
    swapChainInfo_.imageViews.clear();
    swapChainInfo_.images.clear();
}

void Device::destroyCommandPool() {
    commandPool_ = nullptr;
}

void Device::destroyRenderPass() {
    renderPass_ = nullptr;
}

void Device::destroyFrames() {

    if (device_.isNull()) return;

    waitIdle();

    for (auto& frame : frames_) {
        frame.destroy();
    }

}

void Device::freeGraphicsPipelineObjects() {

    if (device_.isNull()) return;

    waitIdle();

    for (auto material : materials_) {
        material->destroy();
    }

    materials_.clear();
    material_ = nullptr;

}

void Device::destroyFrameBuffers() {
    if (device_.isNull()) return;
    waitIdle();
    frameBuffers_.clear();
}

void Device::waitIdle() const {
    if (device_.notNull()) {
        vkDeviceWaitIdle(device_);
    }
}

void Device::beginDraw() {

    VkResult res = VK_SUCCESS;

    auto& frame = frames_[currentFrame_];

    // wait for previous frame
    frame.commandBuffersCompleted.wait();

    uint32_t imageIndex;
    res = vkAcquireNextImageKHR(device_, swapChainInfo_.handle, UINT64_MAX, frame.imageAvailable, VK_NULL_HANDLE, &imageIndex);
    if (VK_ERROR_OUT_OF_DATE_KHR == res) {
        reinitRenderer();
    } else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error(Format::str("Failed to acquire swap chain image: err={}", (int) res));
    }

    frame.commandBuffersCompleted.reset();

    auto& commandBuffer = frame.commandBuffer;
    commandBuffer.reset();

    // record command buffer

    res = commandBuffer.begin();
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to begin recording command buffer: err={}", (int) res));
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass_;
    renderPassInfo.framebuffer = frameBuffers_[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapChainInfo_.extent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (nullptr != material_) {
        material_->bind();
    }

    // dynamic state for viewport
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainInfo_.extent.width;
    viewport.height = (float) swapChainInfo_.extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    // dynamic state for scissor
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainInfo_.extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    currentImageIndex_ = imageIndex;
}

void Device::endDraw() {

    VkResult res = VK_SUCCESS;

    auto& frame = frames_[currentFrame_];

    vkCmdEndRenderPass(frame.commandBuffer);

    res = frame.commandBuffer.end();
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to record command buffer: err={}", (int) res));
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {frame.imageAvailable};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer = frame.commandBuffer;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {frame.renderFinished};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    res = vkQueueSubmit(graphicsQueue_, 1, &submitInfo, frame.commandBuffersCompleted);
    if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to submit draw command buffer: err={}", (int) res));
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChainInfo_.handle};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &currentImageIndex_;
    presentInfo.pResults = nullptr; // Optional

    res = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
        reinitRenderer();
    } else if (VK_SUCCESS != res) {
        throw std::runtime_error(Format::str("Failed to record command buffer: err={}", (int) res));
    }

    currentFrame_ = (currentFrame_ + 1) % frames_.size();
    currentImageIndex_ = 0;
}

const Frame& Device::currentFrame() const {
    return frames_[currentFrame_];
}

bool Device::begin(Window& window) {

    window.getState(windowState_);

    if (!isVisible()) {
        if (!windowState_.minimized) {
            // restore windiow
            reinitRenderer();
        } else {
            // minimized, do not draw
            return false;
        }
    }

    beginDraw();

    return true;
}

bool Device::end() {
    endDraw();
    return true;
}

VkCommandBuffer Device::beginCommand() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool_;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void Device::endCommand(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void Device::drawIndexed(size_t count, size_t offset) {
    const auto& frame = currentFrame();
    const auto& commandBuffer = frame.commandBuffer;
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(count), 1, static_cast<uint32_t>(offset), 0, 0);
}

void Device::draw(size_t count, size_t offset, size_t instances) {
    const auto& frame = currentFrame();
    const auto& commandBuffer = frame.commandBuffer;
    vkCmdDraw(commandBuffer, static_cast<uint32_t>(count), static_cast<uint32_t>(instances), static_cast<uint32_t>(offset), 0);
}

void Device::setMaterial(Material* material) {
    material_ = material;
}


static void errorCallback(const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData) {
    return;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) {

    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT &&
        messageType == VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
        return VK_FALSE;
    }

    const char* severity;
    switch (messageSeverity) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: severity = SEVERITY_VERBOSE; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: severity = SEVERITY_INFO; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: severity = SEVERITY_WARNING; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: severity = SEVERITY_ERROR; break;
        default: severity = SEVERITY_INFO; break;
    }

    const char* type;
    switch (messageType) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: type = "/general"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: type = "/validation"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: type = "/performance"; break;
        default: type = ""; break;
    }

    std::cout << "(" << severity << type << ") " << pCallbackData->pMessage << std::endl;

    if (VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT == messageSeverity) {
        errorCallback(pCallbackData);
    }

    return VK_FALSE;
}
