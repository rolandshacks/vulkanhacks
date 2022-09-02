/*
 * Application
 */
#pragma once

#include <cstdint>

#include "gamekit/device.h"
#include "gamekit/window.h"
#include "gamekit/resources.h"
#include "gamekit/types.h"
#include "gamekit/api.h"

namespace gamekit {

class ApplicationBase {

    private:
        static ApplicationBase* global_instance_;

    protected:
        ApplicationBase(const char* windowTitle, int windowWidth, int windowHeight, int frameRate);
        ~ApplicationBase();

    public:
        static ApplicationBase* globalInstance();

    public:
        virtual void run();

    private:
        void init();
        void shutdown();
        void update();
        void draw();
        void updateStatistics();

    public:
        inline float deltaTime() const { return deltaTime_; }
        inline float absTime() const { return absTime_; }

    protected:
        virtual void createResources() {}
        virtual void destroyResources() {}

    public:
        inline Resources& resources() { return resources_; }
        inline const Resources& resources() const { return resources_; }

    protected:
        virtual void userInit() = 0;
        virtual void userShutdown() = 0;
        virtual void userUpdate() = 0;
        virtual void userDraw() = 0;

    private:
        struct Statistics {
            uint32_t updateCounter{0};
            microsecond_t lastUpdate{0};
            float avgUpdatesPerSecond{0};
        };

    protected:
        Api api_;
        Window window_;
        std::string windowTitle_;
        int windowWidth_{0};
        int windowHeight_{0};
        int frameRate_;
        Device device;
        Statistics stats;
        bool running_{false};
        float deltaTime_{0.0f};
        float absTime_{0.0f};
        Resources resources_;

};

template <
    class T,
    const std::vector<ResourceDescriptor>& (*get_resource_descriptor_fn)()
>
class Application : public ApplicationBase {

    public:
        Application(const char* windowTitle, int windowWidth, int windowHeight, int frameRate) :
            ApplicationBase(windowTitle, windowWidth, windowHeight, frameRate) {}

    protected:
        void createResources() override {
            auto resourceDescriptors = get_resource_descriptor_fn();
            resources_.create(resourceDescriptors);
        }

        void destroyResources() override {
            resources_.destroy();
        }

    protected:
        void userInit() override {
            executive_ = new T();
            executive_->onInit(api_);
        }

        void userShutdown() override {
            if (nullptr != executive_) {
                executive_->onShutdown(api_);
                delete executive_;
                executive_ = nullptr;
            }
        }

        void userUpdate() override {
            executive_->onUpdate(api_);
        }

        void userDraw() override {
            executive_->onDraw(api_);
        }

    private:
        T* executive_{nullptr};

};

} // namespace
