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
    
    void bind();
    void release();
    
    void render();

private:
    
    ProgramObject* _horizontalBlurProgram;
    ProgramObject* _verticalBlurProgram;    
};

#endif /* defined(__interface__GlowEffect__) */
