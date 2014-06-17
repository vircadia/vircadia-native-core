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

const float MAGNIFY_WIDTH = 160.0f;
const float MAGNIFY_HEIGHT = 80.0f;
const float MAGNIFY_MULT = 4.0f;

// Handles the drawing of the overlays to the screen
class ApplicationOverlay {
public:

    ApplicationOverlay();
    ~ApplicationOverlay();

    void renderOverlay(bool renderToTexture = false);
    void displayOverlayTexture();
    void displayOverlayTextureOculus(Camera& whichCamera);
    void computeOculusPickRay(float x, float y, glm::vec3& direction) const;
    void getClickLocation(int &x, int &y) const;

    // Getters
    QOpenGLFramebufferObject* getFramebufferObject();
    float getAlpha() const { return _alpha; }
  
private:
    // Interleaved vertex data
    struct TextureVertex {
        glm::vec3 position;
        glm::vec2 uv;
    };

    enum MousePointerDevice { MOUSE, LEFT_CONTROLLER, RIGHT_CONTROLLER };

    typedef QPair<GLuint, GLuint> VerticesIndices;

    void renderPointers();
    void renderControllerPointers();
    void renderControllerPointersOculus();
    void renderMagnifier(int mouseX, int mouseY, float sizeMult, bool showBorder) const;
    void renderAudioMeter();
    void renderStatsAndLogs();
    void renderTexturedHemisphere();

    QOpenGLFramebufferObject* _framebufferObject;
    float _trailingAudioLoudness;
    float _textureFov;
    // 0 = Mouse, 1 = Left Controller, 2 = Right Controller
    bool _reticleActive[3];
    int _mouseX[3];
    int _mouseY[3];
    bool _magActive[3];
    int _magX[3];
    int _magY[3];
    float _magSizeMult[3];
    
    float _alpha;
    bool _active;

    GLuint _crosshairTexture;
};

#endif // hifi_ApplicationOverlay_h