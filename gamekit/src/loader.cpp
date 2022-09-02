/*
 * Loader
 */

#include "gamekit/loader.h"
#include "gamekit/device.h"
#include "gamekit/utilities.h"

#include <vulkan>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <stdexcept>

using namespace gamekit;

static Loader loader_singleton;

static PFN_vkGetInstanceProcAddr SDL_vkGetInstanceProcAddr = nullptr;
static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL localGetInstanceProcAddr(VkInstance instance, const char* name) {
    return SDL_vkGetInstanceProcAddr(instance, name);
}

Loader* Loader::instance_{nullptr};

Loader* Loader::instance() {
    return instance_;
}

Loader::Loader() {
    if (nullptr == instance_) instance_ = this;
}

Loader::~Loader() {
    if (this == instance_) instance_ = nullptr;
    unload();
}

void Loader::load() {

    if (loaded_) return;

    auto sdl_result = SDL_Vulkan_LoadLibrary(nullptr);
    if (0 != sdl_result) {
        throw std::runtime_error(Format::str("failed to load vulkan library from SDL: err={}", (int) sdl_result));
    }

    // this should not differ from the normal GetProcAddress("vkGetInstanceProcAddr")
    // just to be sure all potential SDL additions are supported
    SDL_vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr> (SDL_Vulkan_GetVkGetInstanceProcAddr());

    volkInitializeCustom(&localGetInstanceProcAddr);

    //volkInitializeCustom(SDL_vkGetInstanceProcAddr);

    loaded_ = true;
}

void Loader::unload() {
    SDL_Vulkan_UnloadLibrary();
    loaded_ = false;
}

void Loader::registerInstance(VkInstance instance) {
    vulkanInstance_ = instance;
    volkLoadInstance(vulkanInstance_);
}

void Loader::unregisterInstance() {
    vulkanInstance_ = nullptr;
}
