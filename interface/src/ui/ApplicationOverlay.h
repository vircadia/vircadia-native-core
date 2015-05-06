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
    void displayOverlayTexture();
    void displayOverlayTextureStereo(Camera& whichCamera, float aspectRatio, float fov);
    void displayOverlayTextureHmd(Camera& whichCamera);

    QPoint getPalmClickLocation(const PalmData *palm) const;
    bool calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const;
    
    bool hasMagnifier() const { return _magnifier; }
    void toggleMagnifier() { _magnifier = !_magnifier; }

    float getHmdUIAngularSize() const { return _hmdUIAngularSize; }
    void setHmdUIAngularSize(float hmdUIAngularSize) { _hmdUIAngularSize = hmdUIAngularSize; }

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

    static glm::vec2 directionToSpherical(const glm::vec3 & direction);
    static glm::vec3 sphericalToDirection(const glm::vec2 & sphericalPos);
    static glm::vec2 screenToSpherical(const glm::vec2 & screenPos);
    static glm::vec2 sphericalToScreen(const glm::vec2 & sphericalPos);
    static void computeHmdPickRay(glm::vec2 cursorPos, glm::vec3& origin, glm::vec3& direction);
    
private:
    // Interleaved vertex data
    struct TextureVertex {
        glm::vec3 position;
        glm::vec2 uv;
    };

    typedef QPair<GLuint, GLuint> VerticesIndices;
    class TexturedHemisphere {
    public:
        TexturedHemisphere();
        ~TexturedHemisphere();
        
        void bind();
        void release();
        GLuint getTexture();
        
        void buildFramebufferObject();
        void buildVBO(const float fov, const float aspectRatio, const int slices, const int stacks);
        void render();
        
    private:
        void cleanupVBO();
        
        GLuint _vertices;
        GLuint _indices;
        QOpenGLFramebufferObject* _framebufferObject;
        VerticesIndices _vbo;
    };
    
    float _hmdUIAngularSize = DEFAULT_HMD_UI_ANGULAR_SIZE;
    
    void renderReticle(glm::quat orientation, float alpha);
    void renderPointers();;
    void renderMagnifier(glm::vec2 magPos, float sizeMult, bool showBorder);
    
    void renderControllerPointers();
    void renderPointersOculus(const glm::vec3& eyePos);
    
    void renderAudioMeter();
    void renderCameraToggle();
    void renderStatsAndLogs();
    void renderDomainConnectionStatusBorder();

    TexturedHemisphere _overlays;
    
    float _textureFov;
    float _textureAspectRatio;
    
    enum Reticles { MOUSE, LEFT_CONTROLLER, RIGHT_CONTROLLER, NUMBER_OF_RETICLES };
    bool _reticleActive[NUMBER_OF_RETICLES];
    QPoint _reticlePosition[NUMBER_OF_RETICLES];
    bool _magActive[NUMBER_OF_RETICLES];
    float _magSizeMult[NUMBER_OF_RETICLES];
    quint64 _lastMouseMove;
    bool _magnifier;

    float _alpha = 1.0f;
    float _oculusUIRadius;
    float _trailingAudioLoudness;


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
