/*
 * Utilities
 */

#include "gamekit/clock.h"
#include "gamekit/types.h"

#include <chrono>
#include <thread>

using namespace gamekit;

microsecond_t Clock::time_offset{0};

void Clock::sleep(microsecond_t micros) {
    if (micros <= 0) return;
    std::this_thread::sleep_for(std::chrono::microseconds(micros));
}

microsecond_t Clock::getTime() {
    //auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto t = std::chrono::duration_cast<std::chrono::microseconds>(duration).count();

    if (0 == time_offset) {
        time_offset = t;
    }

    return (t - time_offset);
}
