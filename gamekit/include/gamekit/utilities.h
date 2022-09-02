/*
 * Types
 */
#pragma once

#include <gamekit/types.h>

#include <string>

namespace gamekit {

class Format {
    public:
        static std::string str(const char* text, int errorCode);
        static std::string str(const char* text, const char* err);
};

class Random {
    public:
        static const int MAX = RAND_MAX;

    public:
        static int getInt();
        static int getInt(int rangeMin, int rangeMax);

        static float getFloat();
        static float getFloat(float rangeMin, float rangeMax);

    private:
        static bool initialize();

    private:
        static bool initialized_;
};

class Environment {

    public:
        static const std::string& getBasePath();
        static std::string getPrefPath(const std::string& org, const std::string& app);

};

} // namespace
