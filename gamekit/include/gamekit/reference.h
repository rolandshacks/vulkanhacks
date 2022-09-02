/*
 * Types
 */
#pragma once

#include <type_traits>

namespace gamekit {

///////////////////////////////////////////////////////////////////////////////
// Reference
///////////////////////////////////////////////////////////////////////////////

template <typename T>
class Reference {

    public:
        Reference() {}

        Reference(T handle) : handle_{handle} {}

        Reference& operator=(T handle) {
            free();
            handle_ = handle;
            return *this;
        }

        Reference(const Reference& ref) = delete;

        Reference& operator=(const Reference&) = delete;

        Reference(Reference&& ref) {
            handle_ = ref.handle_; ref.handle_ = nullptr;
            attached_ = ref.attached_; ref.attached_ = false;
        }

        Reference& operator=(Reference&& ref) {
            if (this == &ref) {
                return *this;
            }

            free();
            handle_ = ref.handle_; ref.handle_ = nullptr;
            attached_ = ref.attached_; ref.attached_ = false;

            return *this;
        }

        virtual ~Reference() {
            free();
        }

    public:
        void free() {
            if constexpr (std::is_pointer_v<T>) {
                if (nullptr != handle_) {
                    if (false == attached_) destroy();
                    handle_ = nullptr;
                }
            } else {
                if (false == attached_) destroy();
                handle_ = T{};
            }

            attached_ = false;
        }

        void attach(T handle) {
            handle_ = handle;
            attached_ = true;
        }

        [[nodiscard]] bool isNull() const {
            if constexpr (std::is_pointer_v<T>) {
                return (nullptr == handle_);
            } else {
                return false;
            }

        }

        [[nodiscard]] bool notNull() const {
            if constexpr (std::is_pointer_v<T>) {
                return (nullptr != handle_);
            } else {
                return true;
            }

        }

    protected:
        void destroy() {};

    public:
        [[nodiscard]] T ptr() {
            return handle_;
        }

        [[nodiscard]] const T ptr() const {
            return handle_;
        }

        [[nodiscard]] T* ref_ptr() {
            return &handle_;
        }

        [[nodiscard]] const T* ref_ptr() const {
            return &handle_;
        }

        operator T() const {
            return handle_;
        }

    protected:
        T handle_{};
        bool attached_{false};

};

} // namespace
