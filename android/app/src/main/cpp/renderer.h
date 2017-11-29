#pragma once

#include <chrono>
#include <array>
#include <glm/glm.hpp>

class NativeRenderer {
public:
    void InitializeGl();
    void DrawFrame();
    void OnTriggerEvent();
    void OnPause();
    void OnResume();

private:
    std::chrono::time_point<std::chrono::system_clock> start { std::chrono::system_clock::now() };

    uint32_t _geometryBuffer { 0 };
    uint32_t _vao { 0 };
    uint32_t _program { 0 };
};
