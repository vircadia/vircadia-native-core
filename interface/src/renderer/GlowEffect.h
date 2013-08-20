//
//  GlowEffect.h
//  interface
//
//  Created by Andrzej Kapolka on 8/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__GlowEffect__
#define __interface__GlowEffect__

#include <QObject>

class QOpenGLFramebufferObject;

class ProgramObject;

/// A generic full screen glow effect.
class GlowEffect : public QObject {
    Q_OBJECT
    
public:
    
    GlowEffect();
    
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
    
    /// Renders the glow effect.  To be called after rendering the scene.
    void render();

public slots:
    
    void cycleRenderMode();
    
private:
    
    enum RenderMode { ADD_MODE, BLUR_ADD_MODE, BLUR_PERSIST_ADD_MODE, DIFFUSE_ADD_MODE, RENDER_MODE_COUNT };
    
    RenderMode _renderMode;
    ProgramObject* _addProgram;
    ProgramObject* _horizontalBlurProgram;
    ProgramObject* _verticalBlurAddProgram;
    ProgramObject* _verticalBlurProgram;
    ProgramObject* _addSeparateProgram;
    ProgramObject* _diffuseProgram;
    
    bool _isEmpty; ///< set when nothing in the scene is currently glowing
    bool _isOddFrame; ///< controls the alternation between texture targets in diffuse add mode
};

#endif /* defined(__interface__GlowEffect__) */
