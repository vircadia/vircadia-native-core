//
//  GlowEffect.h
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 8/7/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_GlowEffect_h
#define hifi_GlowEffect_h

#include <QObject>
#include <QStack>

class QOpenGLFramebufferObject;

class ProgramObject;

/// A generic full screen glow effect.
class GlowEffect : public QObject {
    Q_OBJECT
    
public:
    GlowEffect();
    ~GlowEffect();
    
    /// Returns a pointer to the framebuffer object that the glow effect is *not* using for persistent state
    /// (either the secondary or the tertiary).
    QOpenGLFramebufferObject* getFreeFramebufferObject() const;
    
    void init();
    
    /// Prepares the glow effect for rendering the current frame.  To be called before rendering the scene.
    void prepare();
    
    /// Starts using the glow effect.
    /// \param intensity the desired glow intensity, from zero to one
    void begin(float intensity = 1.0f);
    
    /// Stops using the glow effect.
    void end();
    
    /// Returns the current glow intensity.
    float getIntensity() const { return _intensity; }
    
    /// Renders the glow effect.  To be called after rendering the scene.
    /// \param toTexture whether to render to a texture, rather than to the frame buffer
    /// \return the framebuffer object to which we rendered, or NULL if to the frame buffer
    QOpenGLFramebufferObject* render(bool toTexture = false);

private:

    bool _initialized;

    ProgramObject* _addProgram;
    ProgramObject* _horizontalBlurProgram;
    ProgramObject* _verticalBlurAddProgram;
    ProgramObject* _verticalBlurProgram;
    ProgramObject* _addSeparateProgram;
    ProgramObject* _diffuseProgram;
    int _diffusionScaleLocation;
    
    bool _isEmpty; ///< set when nothing in the scene is currently glowing
    bool _isOddFrame; ///< controls the alternation between texture targets in diffuse add mode
    bool _isFirstFrame; ///< for persistent modes, notes whether this is the first frame rendered
    
    float _intensity;
    QStack<float> _intensityStack;
};

/// RAII-style glow handler.  Applies glow when in scope.
class Glower {
public:
    
    Glower(float amount = 1.0f);
    ~Glower();
};

#endif // hifi_GlowEffect_h
