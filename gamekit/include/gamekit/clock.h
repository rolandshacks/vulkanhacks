/*
 * Utilities
 */
#pragma once

#include <vulkan>

#include "gamekit/types.h"
#include "gamekit/clock.h"

namespace gamekit {

class Clock {

    public:
        static void sleep(microsecond_t micros);
        static microsecond_t getTime();

    private:
        static microsecond_t time_offset;

};

}
