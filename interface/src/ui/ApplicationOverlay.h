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

#include <gpu/Texture.h>
class Camera;
class Overlays;
class QOpenGLFramebufferObject;
class QOpenGLTexture;

const float MAGNIFY_WIDTH = 220.0f;
const float MAGNIFY_HEIGHT = 100.0f;
const float MAGNIFY_MULT = 2.0f;

const float DEFAULT_HMD_UI_ANGULAR_SIZE = 72.0f;

// Handles the drawing of the overlays to the screen
// TODO, move divide up the rendering, displaying and input handling
// facilities of this class
class ApplicationOverlay : public QObject {
    Q_OBJECT
public:
    ApplicationOverlay();
    ~ApplicationOverlay();

    void renderOverlay();
    GLuint getOverlayTexture();
    
    QPoint getPalmClickLocation(const PalmData *palm) const;
    bool calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const;
    
    bool hasMagnifier() const { return _magnifier; }
    void toggleMagnifier() { _magnifier = !_magnifier; }

    // Converter from one frame of reference to another.
    // Frame of reference:
    // Direction: Ray that represents the spherical values
    // Screen: Position on the screen (x,y)
    // Spherical: Pitch and yaw that gives the position on the sphere we project on (yaw,pitch)
    // Overlay: Position on the overlay (x,y)
    // (x,y) in Overlay are similar than (x,y) in Screen except they can be outside of the bound of te screen.
    // This allows for picking outside of the screen projection in 3D.
    glm::vec2 sphericalToOverlay(const glm::vec2 & sphericalPos) const;
    glm::vec2 overlayToSpherical(const glm::vec2 & overlayPos) const;
    glm::vec2 screenToOverlay(const glm::vec2 & screenPos) const;
    glm::vec2 overlayToScreen(const glm::vec2 & overlayPos) const;
    void computeHmdPickRay(glm::vec2 cursorPos, glm::vec3& origin, glm::vec3& direction) const;

    static glm::vec2 directionToSpherical(const glm::vec3 & direction);
    static glm::vec3 sphericalToDirection(const glm::vec2 & sphericalPos);
    static glm::vec2 screenToSpherical(const glm::vec2 & screenPos);
    static glm::vec2 sphericalToScreen(const glm::vec2 & sphericalPos);
    
private:
    
    void renderReticle(glm::quat orientation, float alpha);
    void renderPointers();;
    void renderMagnifier(glm::vec2 magPos, float sizeMult, bool showBorder);
    
    void renderControllerPointers();
    void renderPointersOculus(const glm::vec3& eyePos);
    
    void renderAudioMeter();
    void renderCameraToggle();
    void renderStatsAndLogs();
    void renderDomainConnectionStatusBorder();
    
    enum Reticles { MOUSE, LEFT_CONTROLLER, RIGHT_CONTROLLER, NUMBER_OF_RETICLES };
    bool _reticleActive[NUMBER_OF_RETICLES];
    QPoint _reticlePosition[NUMBER_OF_RETICLES];
    bool _magActive[NUMBER_OF_RETICLES];
    float _magSizeMult[NUMBER_OF_RETICLES];
    quint64 _lastMouseMove;
    bool _magnifier;
    float _hmdUIRadius{ 1.0 };


    float _alpha = 1.0f;
    float _trailingAudioLoudness;
    QOpenGLFramebufferObject* _framebufferObject{nullptr};

    gpu::TexturePointer _crosshairTexture;
    GLuint _newUiTexture{ 0 };
    
    int _reticleQuad;
    int _magnifierQuad;
    int _audioRedQuad;
    int _audioGreenQuad;
    int _audioBlueQuad;
    int _domainStatusBorder;
    int _magnifierBorder;

    int _previousBorderWidth;
    int _previousBorderHeight;

    glm::vec3 _previousMagnifierBottomLeft;
    glm::vec3 _previousMagnifierBottomRight;
    glm::vec3 _previousMagnifierTopLeft;
    glm::vec3 _previousMagnifierTopRight;
};

#endif // hifi_ApplicationOverlay_h
