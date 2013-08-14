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

class ProgramObject;

class GlowEffect : public QObject {
    Q_OBJECT
    
public:
    
    GlowEffect();
    
    void init();
    
    void prepare();
    
    void begin(float amount = 1.0f);
    void end();
    
    void render();

public slots:
    
    void cycleRenderMode();
    
private:
    
    enum RenderMode { ADD_MODE, BLUR_ADD_MODE, BLUR_PERSIST_ADD_MODE, RENDER_MODE_COUNT };
    
    RenderMode _renderMode;
    ProgramObject* _addProgram;
    ProgramObject* _horizontalBlurProgram;
    ProgramObject* _verticalBlurAddProgram;
    ProgramObject* _verticalBlurProgram;
    ProgramObject* _addSeparateProgram;
    
    bool _isEmpty;
};

#endif /* defined(__interface__GlowEffect__) */
