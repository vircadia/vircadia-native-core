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

// Handles the drawing of the overlays to the screen
class ApplicationOverlay {
public:

    ApplicationOverlay();
    ~ApplicationOverlay();

    void renderOverlay(bool renderToTexture = false);
    void displayOverlayTexture(Camera& whichCamera);
    void displayOverlayTextureOculus(Camera& whichCamera);
    void computeOculusPickRay(float x, float y, glm::vec3& direction) const;

    // Getters
    QOpenGLFramebufferObject* getFramebufferObject();
  
private:
    // Interleaved vertex data
    struct TextureVertex {
        glm::vec3 position;
        glm::vec2 uv;
    };

    typedef QPair<GLuint, GLuint> VerticesIndices;

    void renderPointers();
    void renderControllerPointers();
    void renderControllerPointersOculus();
    void renderMagnifier(int mouseX, int mouseY);
    void renderAudioMeter();
    void renderStatsAndLogs();
    void renderTexturedHemisphere();

    QOpenGLFramebufferObject* _framebufferObject;
    float _trailingAudioLoudness;
    float _oculusAngle;
    float _distance;
    float _textureFov;
    int _mouseX[2];
    int _mouseY[2];
    int _numMagnifiers;
    
    GLuint _crosshairTexture;
};

#endif // hifi_ApplicationOverlay_h