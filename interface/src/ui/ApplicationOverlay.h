//
//  ApplicationOverlay.h
//  interface/src/ui/overlays
//
//  Created by Benjamin Arnold on 5/27/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ApplicationOverlay_h
#define hifi_ApplicationOverlay_h

class Overlays;
class QOpenGLFramebufferObject;

// Handles the drawing of the overlays to the scree
class ApplicationOverlay {
public:

    static enum UITYPES { HEMISPHERE, SEMICIRCLE, CURVED_SEMICIRCLE };

    ApplicationOverlay();
    ~ApplicationOverlay();

    void renderOverlay(bool renderToTexture = false);
    void displayOverlayTexture(Camera& whichCamera);
    void displayOverlayTextureOculus(Camera& whichCamera);

    // Getters
    QOpenGLFramebufferObject* getFramebufferObject();
    float getOculusAngle() const { return _oculusAngle; }

    // Setters
    void setOculusAngle(float oculusAngle) { _oculusAngle = oculusAngle; }
    void setUiType(UITYPES uiType) { _uiType = uiType; }

private:
    // Interleaved vertex data
    struct TextureVertex {
        glm::vec3 position;
        glm::vec2 uv;
    };

    typedef QPair<GLuint, GLuint> VerticesIndices;

    void renderTexturedHemisphere();

    QOpenGLFramebufferObject* _framebufferObject;
    float _trailingAudioLoudness;
    float _oculusAngle;
    float _distance;
    int _uiType;
};

#endif // hifi_ApplicationOverlay_h