/*
 * Window Toolkit
 */

#include <vulkan>

#include "gamekit/window.h"
#include "gamekit/loader.h"
#include "gamekit/utilities.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <string.h>
#include <stdexcept>

using namespace gamekit;

void Window::create(const char* title, int width, int height) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    Loader::instance()->load();

    uint32_t windowFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    auto window = SDL_CreateWindow(title,
                                   SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                   width, height,
                                   windowFlags);

    handle_ = static_cast<void*>(window);

}

void Window::destroy() {
    if (nullptr == handle_) return;

    auto window = static_cast<SDL_Window*>(handle_);

    if (nullptr != handle_) {
        SDL_DestroyWindow(window);
        handle_ = nullptr;
    }

    Loader::instance()->unload();

    SDL_Quit();

    handle_ = nullptr;
}

void* Window::handle() const {
    return handle_;
}

void Window::getState(Window::WindowState& state) const {
    auto window = static_cast<SDL_Window*>(handle_);
    SDL_Vulkan_GetDrawableSize(window, &state.width, &state.height);
    uint32_t windowFlags = SDL_GetWindowFlags(window);
    state.minimized = ((windowFlags & SDL_WINDOW_MINIMIZED) != 0);
}

std::vector<const char*> Window::getVulkanExtensions() const {
    auto window = static_cast<SDL_Window*>(handle_);

    unsigned int extensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    if (0 == extensionCount) {
        throw std::runtime_error("Failed to find retrieve SDL vulkan extensions");
    }

    std::vector<const char*> extensionNames(extensionCount);
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames.data());

    return extensionNames;
}

void Window::createVulkanSurface(const VkInstance& instance, VkSurfaceKHR* surface) const {
    auto window = static_cast<SDL_Window*>(handle_);
    if (SDL_TRUE != SDL_Vulkan_CreateSurface(window, instance, surface)) {
        throw std::runtime_error(Format::str("Failed to create surface: err={}", SDL_GetError()));
    }
}

bool Window::processEvents() const {

    bool ok = true;

    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            ok = false;
        } else if (event.type == SDL_KEYUP) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                ok = false;
            }
        }
    }

    return ok;
}

void Window::waitEvents() const {
    SDL_WaitEventTimeout(NULL, 100);
}
