#pragma once
#include <cstdint>
#include <string>
#include "Logger.h"

#define MODEL_PATH "../assets/models/viking_room.obj"
#define TEXTURE_PATH "../assets/textures/viking_room.png"

struct Config {
    uint32_t windowWidth = 1280;
    uint32_t windowHeight = 800;
    std::string windowTitle = "G.L. Engine";
    uint32_t maxFramesInFlight = NULL;
    bool enableValidation = true;
	bool enableHeadlessSurface = false;
    bool enableGui = false;
    std::string fontPath = "../assets/fonts/Roboto-Regular.ttf";
    float fontSize = 16.0f;
    uint64_t maxStagingBufferSize = 128ull * 1024ull * 1024ull;
};