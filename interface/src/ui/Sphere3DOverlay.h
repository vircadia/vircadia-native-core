//
//  Sphere3DOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Sphere3DOverlay__
#define __interface__Sphere3DOverlay__

#include "Volume3DOverlay.h"

class Sphere3DOverlay : public Volume3DOverlay {
    Q_OBJECT
    
public:
    Sphere3DOverlay();
    ~Sphere3DOverlay();
    virtual void render();
};

 
#endif /* defined(__interface__Sphere3DOverlay__) */
