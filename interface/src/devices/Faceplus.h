//
//  Faceplus.h
//  interface
//
//  Created by Andrzej Kapolka on 4/8/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__Faceplus__
#define __interface__Faceplus__

#include "FaceTracker.h"

/// Interface for Mixamo FacePlus.
class Faceplus : public FaceTracker {
    Q_OBJECT
    
public:
    
    Faceplus();
    
    void init();
    
    bool isActive() const { return _active; }
    
    void update();
    void reset();

private:
    
    bool _active;
};

#endif /* defined(__interface__Faceplus__) */
