//
//  GlowEffect.h
//  interface
//
//  Created by Andrzej Kapolka on 8/7/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__GlowEffect__
#define __interface__GlowEffect__

class ProgramObject;

class GlowEffect {
public:
    
    void init();
    
    void prepare();
    
    void begin(float amount = 1.0f);
    void end();
    
    void render();

private:
    
    ProgramObject* _horizontalBlurProgram;
    ProgramObject* _verticalBlurProgram;
    
    bool _isEmpty;
};

#endif /* defined(__interface__GlowEffect__) */
