/*
 * Application
 */

#include "gamekit/application.h"
#include "gamekit/device.h"
#include "gamekit/types.h"
#include "gamekit/clock.h"
#include "gamekit/api.h"

#include <iostream>

const bool enableErrorChecking = true;

using namespace gamekit;

ApplicationBase* ApplicationBase::global_instance_{nullptr};


ApplicationBase::ApplicationBase(const char* windowTitle, int windowWidth, int windowHeight, int frameRate) :
    windowTitle_{windowTitle},
    windowWidth_{windowWidth},
    windowHeight_{windowHeight},
    frameRate_{frameRate} {
    if (nullptr == global_instance_) {
        global_instance_ = this;
    }
}

ApplicationBase::~ApplicationBase() {
    if (this == global_instance_) {
        global_instance_ = nullptr;
    }
}

ApplicationBase* ApplicationBase::globalInstance() {
    return global_instance_;
}

void ApplicationBase::init() {

    running_ = false;

    window_.create(windowTitle_.c_str(), windowWidth_, windowHeight_);
    device.createDevice(window_, enableErrorChecking);

    createResources();

    api_.create();

    userInit();

    device.createRenderer();
}

void ApplicationBase::shutdown() {
    running_ = false;

    device.waitIdle();

    destroyResources();

    device.destroyRenderer();

    userShutdown();

    api_.destroy();

    device.destroyDevice();
    window_.destroy();
}

void ApplicationBase::run() {

    init();

    running_ = true;

    const microsecond_t cycle_time = 1000000LL / (int64_t) frameRate_;
    const microsecond_t min_cycle_time = 5000;
    const microsecond_t max_sleep_time = 10000;
    microsecond_t next_cycle = 0;
    microsecond_t lastUpdateTime = 0;

    microsecond_t now = 0;

    while (running_) {

        while (running_) {

            if (!window_.processEvents()) {
                running_ = false;
                break;
            }

            now = Clock::getTime();
            if (now >= next_cycle) {
                next_cycle += cycle_time;
                if (next_cycle < now + min_cycle_time) {
                    next_cycle = now + min_cycle_time;
                }
                break;
            }

            auto sleep_time = next_cycle - now;
            Clock::sleep(sleep_time > max_sleep_time ? max_sleep_time : sleep_time);
        }

        absTime_ = (float) now * 0.000001f;
        auto delta = (lastUpdateTime != 0) ? now - lastUpdateTime : 0;
        deltaTime_ = (float) delta / 1000000.0f;
        lastUpdateTime = now;

        update();

        if (device.begin(window_)) {
            draw();
            device.end();
            updateStatistics();
        } else {
            // no drawing (minimized) -> idle state - waiting for events
            window_.waitEvents();
        }

    }

    shutdown();
}

void ApplicationBase::update() {
    userUpdate();
}

void ApplicationBase::draw() {
    userDraw();
}

void ApplicationBase::updateStatistics() {

    stats.updateCounter++;

    auto now = Clock::getTime();
    auto delta = (float) (now - stats.lastUpdate) / 1000000.0f;

    if (delta >= 5.0) {
        stats.avgUpdatesPerSecond = (float) stats.updateCounter / delta;
        stats.updateCounter = 0;
        stats.lastUpdate = now;
        std::cout << "fps: " << stats.avgUpdatesPerSecond << std::endl;
    }
}
