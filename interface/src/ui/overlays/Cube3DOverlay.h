//
//  Cube3DOverlay.h
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
