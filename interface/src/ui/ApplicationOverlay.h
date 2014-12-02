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
    void displayOverlayTexture3DTV(Camera& whichCamera, float aspectRatio, float fov);
    void computeOculusPickRay(float x, float y, glm::vec3& direction) const;
    void getClickLocation(int &x, int &y) const;
    QPoint getPalmClickLocation(const PalmData *palm) const;
    bool calculateRayUICollisionPoint(const glm::vec3& position, const glm::vec3& direction, glm::vec3& result) const;


    // Getters
    float getAlpha() const { return _alpha; }
  
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
        
        void buildVBO(const float fov, const float aspectRatio, const int slices, const int stacks);
        void buildFramebufferObject(const QSize& size);
        void render(const glm::quat& orientation, const glm::vec3& position, const float scale);
        
    private:
        void cleanupVBO();
        
        GLuint _vertices;
        GLuint _indices;
        QOpenGLFramebufferObject* _framebufferObject;
        VerticesIndices _vbo;
    };
    
    VerticesIndices* makeTexturedHemiphereVBO(float fov, float aspectRatio, int slices, int stacks);
    void renderTexturedHemisphere(ApplicationOverlay::VerticesIndices* vbo, int vertices, int indices);
    QOpenGLFramebufferObject* newFramebufferObject(QSize& size);
    
    void renderPointers();
    void renderControllerPointers();
    void renderPointersOculus(const glm::vec3& eyePos);
    void renderMagnifier(int mouseX, int mouseY, float sizeMult, bool showBorder) const;
    void renderAudioMeter();
    void renderStatsAndLogs();
    void renderDomainConnectionStatusBorder();

    TexturedHemisphere _overlays;
    TexturedHemisphere _reticule;
    
    float _trailingAudioLoudness;
    float _textureFov;
    
    enum MagnifyDevices { MOUSE, LEFT_CONTROLLER, RIGHT_CONTROLLER, NUMBER_OF_MAGNIFIERS };
    bool _reticleActive[NUMBER_OF_MAGNIFIERS];
    int _mouseX[NUMBER_OF_MAGNIFIERS];
    int _mouseY[NUMBER_OF_MAGNIFIERS];
    bool _magActive[NUMBER_OF_MAGNIFIERS];
    int _magX[NUMBER_OF_MAGNIFIERS];
    int _magY[NUMBER_OF_MAGNIFIERS];
    float _magSizeMult[NUMBER_OF_MAGNIFIERS];
    
    float _alpha;
    float _oculusUIRadius;

    GLuint _crosshairTexture;
};

#endif // hifi_ApplicationOverlay_h
