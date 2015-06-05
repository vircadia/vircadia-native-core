
#pragma once

#include <gpu/Texture.h>
#include <display-plugins/DisplayPlugin.h>
#include <display-plugins/OglplusHelpers.h>
#include <OffscreenGlCanvas.h>

class ApplicationOverlayCompositor {
public:
    ApplicationOverlayCompositor();
    GLuint composite(DisplayPlugin* plugin,
            GLuint sceneTexture, const glm::uvec2& sceneSize,
            GLuint overlayTexture, const glm::uvec2& overlaySize);

private:
    OffscreenGlCanvas _canvas;
    BasicFramebufferWrapperPtr _fbo;
    ProgramPtr          _program;
    ShapeWrapperPtr     _plane;
    ShapeWrapperPtr     _hmdUiSurface;
    gpu::TexturePointer _crosshairTexture;

    void composite2D(DisplayPlugin* plugin,
            GLuint sceneTexture, const glm::uvec2& sceneSize,
            GLuint overlayTexture, const glm::uvec2& overlaySize);

    void compositeStereo(DisplayPlugin* plugin,
            GLuint sceneTexture, const glm::uvec2& sceneSize,
            GLuint overlayTexture, const glm::uvec2& overlaySize);

    void compositeHmd(DisplayPlugin* plugin,
            GLuint sceneTexture, const glm::uvec2& sceneSize,
            GLuint overlayTexture, const glm::uvec2& overlaySize);
};

using CompositorPtr = std::shared_ptr<ApplicationOverlayCompositor>;
