/*
 * Loader
 */
#pragma once

#include <vulkan>

namespace gamekit {

class Loader {

    public:
        static Loader* instance();
        Loader();
        ~Loader();

    public:
        void load();
        void unload();
        void registerInstance(VkInstance instance);
        void unregisterInstance();

    private:
        bool loaded_{false};
        VkInstance vulkanInstance_{nullptr};

    private:
        static Loader* instance_;
};

} // namespace
