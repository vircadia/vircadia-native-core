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

const float MAGNIFY_WIDTH = 220.0f;
const float MAGNIFY_HEIGHT = 100.0f;
const float MAGNIFY_MULT = 2.0f;

// Handles the drawing of the overlays to the screen
class ApplicationOverlay {
public:
    ApplicationOverlay();
    ~ApplicationOverlay();

    void renderOverlay(bool renderToTexture = false);
    void displayOverlayTexture();
    void displayOverlayTextureOculus(Camera& whichCamera);
    void displayOverlayTexture3DTV(Camera& whichCamera, float aspectRatio, float fov);
    
    void computeOculusPickRay(float x, float y, glm::vec3& origin, glm::vec3& direction) const;
    QPoint getPalmClickLocation(const PalmData *palm) const;
    bool calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const;
    
    // Converter from one frame of reference to another.
    // Frame of reference:
    // Screen: Position on the screen (x,y)
    // Spherical: Pitch and yaw that gives the position on the sphere we project on (yaw,pitch)
    // Overlay: Position on the overlay (x,y)
    // (x,y) in Overlay are similar than (x,y) in Screen except they can be outside of the bound of te screen.
    // This allows for picking outside of the screen projection in 3D.
    glm::vec2 screenToSpherical(glm::vec2 screenPos) const;
    glm::vec2 sphericalToScreen(glm::vec2 sphericalPos) const;
    glm::vec2 sphericalToOverlay(glm::vec2 sphericalPos) const;
    glm::vec2 overlayToSpherical(glm::vec2 overlayPos) const;
    glm::vec2 screenToOverlay(glm::vec2 screenPos) const;
    glm::vec2 overlayToScreen(glm::vec2 overlayPos) const;
    
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
        void bindTexture();
        void releaseTexture();
        
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
    
    void renderPointers();;
    void renderMagnifier(glm::vec2 magPos, float sizeMult, bool showBorder) const;
    
    void renderControllerPointers();
    void renderPointersOculus(const glm::vec3& eyePos);
    
    void renderAudioMeter();
    void renderStatsAndLogs();
    void renderDomainConnectionStatusBorder();

    TexturedHemisphere _overlays;
    
    float _textureFov;
    float _textureAspectRatio;
    
    enum Reticules { MOUSE, LEFT_CONTROLLER, RIGHT_CONTROLLER, NUMBER_OF_RETICULES };
    bool _reticleActive[NUMBER_OF_RETICULES];
    glm::vec2 _reticulePosition[NUMBER_OF_RETICULES];
    bool _magActive[NUMBER_OF_RETICULES];
    float _magSizeMult[NUMBER_OF_RETICULES];
    
    float _alpha;
    float _oculusUIRadius;
    float _trailingAudioLoudness;

    GLuint _crosshairTexture;
};

#endif // hifi_ApplicationOverlay_h
