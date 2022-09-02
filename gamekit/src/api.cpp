/*
 * Gamekit API
 */

#include "gamekit/api.h"

#include "gamekit/application.h"
#include "gamekit/device.h"

#include <stdexcept>

using namespace gamekit;

struct Api::Context {
    ApplicationBase* application_{nullptr};
    Device* device_{nullptr};

    Context(ApplicationBase* application, Device* device) :
        application_(application),
        device_(device) {}
};

// shortcut calls
#define application context_->application_
#define device context_->device_

void Api::create() {
    context_ = new Api::Context(
        ApplicationBase::globalInstance(),
        Device::globalInstance()
    );
}

void Api::destroy() {
    if (nullptr != context_) {
        delete context_;
        context_ = nullptr;
    }
}

float Api::deltaTime() const {
    return application->deltaTime();
}

float Api::absTime() const {
    return application->absTime();
}

void Api::addMaterial(Material& material) {
    device->addMaterial(material);
}

void Api::setMaterial(Material* material) {
    device->setMaterial(material);
}

Material* Api::material() {
    return device->material();
}

const Metrics& Api::metrics() const {
    return device->metrics();
}

const Frame& Api::currentFrame() const {
    return device->currentFrame();
}

const Resources& Api::resources() const {
    return application->resources();
}

Resources& Api::resources() {
    return application->resources();
}

const ResourceDescriptor& Api::resources(const std::string& id) const {
    return application->resources().get(id);
}
