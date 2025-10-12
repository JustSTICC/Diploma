#pragma once
#include <cstdint>
#include <string>
#include "Logger.h"

struct Config {
    uint32_t windowWidth = 800;
    uint32_t windowHeight = 600;
    std::string windowTitle = "G.L. Engine";
    uint32_t maxFramesInFlight = 2;
    bool enableValidation = true;
    bool enableGui = true;
    std::string fontPath = "";
    float fontSize = 16.0f;
};