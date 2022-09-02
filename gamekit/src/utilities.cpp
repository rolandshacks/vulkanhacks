/*
 * Utilities
 */

#include "gamekit/utilities.h"

#include <SDL2/SDL.h>

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>

using namespace gamekit;

std::string Format::str(const char* text, int errorCode) {
    std::ostringstream ss;
    ss << text << ", err=" << errorCode;
    return ss.str();
}

std::string Format::str(const char* text, const char* err) {
    std::ostringstream ss;
    ss << text << ", err=" << err;
    return ss.str();
}

int Random::getInt() {
    return std::rand();
}

int Random::getInt(int rangeMin, int rangeMax) {
    if (rangeMax <= rangeMin) return rangeMin;

    int range = rangeMax - rangeMin;
    return rangeMin + (int) (((int64_t) rand() * (int64_t) range) / (int64_t) RAND_MAX);
}

float Random::getFloat() {
    return (float) std::rand() / (float) RAND_MAX;
}

float Random::getFloat(float rangeMin, float rangeMax) {
    if (rangeMax <= rangeMin) return rangeMin;
    float range = rangeMax - rangeMin;
    return rangeMin + ((float) rand() * range) / (float) RAND_MAX;
}

bool Random::initialize() {
    std::srand((unsigned int) std::time(nullptr));
    return true;
}

bool Random::initialized_ = Random::initialize();


struct EnvironmentData {

    public:
        EnvironmentData() {
            basePath = SDL_GetBasePath();
            std::cout << "BASE PATH : " << basePath << std::endl;
        }

    public:
        std::string basePath;
        std::string prefPath;
};

static EnvironmentData environmentData;

const std::string& Environment::getBasePath() {
    return environmentData.basePath;
}

std::string Environment::getPrefPath(const std::string& org, const std::string& app) {
    return SDL_GetPrefPath(org.c_str(), app.c_str());
}
