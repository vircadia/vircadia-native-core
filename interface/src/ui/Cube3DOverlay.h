//
//  Cube3DOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Cube3DOverlay__
#define __interface__Cube3DOverlay__

#include "Volume3DOverlay.h"

class Cube3DOverlay : public Volume3DOverlay {
    Q_OBJECT
    
public:
    Cube3DOverlay();
    ~Cube3DOverlay();
    virtual void render();
};

 
#endif /* defined(__interface__Cube3DOverlay__) */
