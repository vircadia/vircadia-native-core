#pragma once

#include <chrono>
#include <array>
#include <glm/glm.hpp>

//#define GVR

#if defined(GVR)
#include <vr/gvr/capi/include/gvr.h>
#endif

class NativeRenderer {
public:

#if defined(GVR)
    NativeRenderer(gvr_context* vrContext);
#else
    NativeRenderer(void* vrContext);
#endif

    void InitializeGl();
    void DrawFrame();
    void OnTriggerEvent();
    void OnPause();
    void OnResume();

private:


    std::chrono::time_point<std::chrono::system_clock> start;
#if defined(GVR)
    void InitializeVR();
    void PrepareFramebuffer();

    std::unique_ptr<gvr::GvrApi> _gvrapi;
    gvr::ViewerType _gvr_viewer_type;
    std::unique_ptr<gvr::BufferViewportList> _viewportlist;
    std::unique_ptr<gvr::SwapChain> _swapchain;
    std::array<gvr::BufferViewport, 2> _viewports;
    gvr::Sizei _renderSize;
#endif

    uint32_t _cubeBuffer { 0 };
    uint32_t _cubeVao { 0 };
    uint32_t _cubeProgram { 0 };

    glm::mat4 _head_view;
    glm::mat4 _model_cube;
    glm::mat4 _camera;
    glm::mat4 _view;
    glm::mat4 _model_floor;

    std::array<glm::mat4, 2> _modelview_cube;
    std::array<glm::mat4, 2> _modelview_floor;
    std::array<glm::mat4, 2> _modelview_projection_cube;
    std::array<glm::mat4, 2> _modelview_projection_floor;
    std::array<glm::vec3, 2> _light_pos_eye_space;
    const glm::vec4 _light_pos_world_space{ 0, 2, 0, 1};
};
