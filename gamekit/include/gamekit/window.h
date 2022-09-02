/*
 * Window Toolkit
 */
#pragma once

#include <vulkan>
#include <vector>

namespace gamekit {

class Window {

public:
    struct WindowState {
        int width;
        int height;
        bool minimized;
    };

public:
    void create(const char* title, int width, int height);
    void destroy();

    std::vector<const char*> getVulkanExtensions() const;
    void createVulkanSurface(const VkInstance& instance, VkSurfaceKHR* surface) const;

    bool processEvents() const;
    void waitEvents() const;

public:
    void* handle() const;
    void getState(WindowState& windowState) const;

private:
    void* handle_{nullptr};

};

} // namespace
