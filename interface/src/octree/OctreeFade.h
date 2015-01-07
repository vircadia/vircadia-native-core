//
//  OctreeFade.h
//  interface/src/octree
//
//  Created by Brad Hefta-Gaub on 8/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeFade_h
#define hifi_OctreeFade_h

#include <OctalCode.h> // for VoxelPositionSize

class OctreeFade {
public:

    enum FadeDirection { FADE_OUT, FADE_IN};
    static const float FADE_OUT_START;
    static const float FADE_OUT_END;
    static const float FADE_OUT_STEP;
    static const float FADE_IN_START;
    static const float FADE_IN_END;
    static const float FADE_IN_STEP;
    static const float DEFAULT_RED;
    static const float DEFAULT_GREEN;
    static const float DEFAULT_BLUE;

    VoxelPositionSize   voxelDetails;
    FadeDirection       direction;
    float               opacity;
    
    float               red;
    float               green;
    float               blue;
    
    OctreeFade(FadeDirection direction = FADE_OUT, float red = DEFAULT_RED, 
              float green = DEFAULT_GREEN, float blue = DEFAULT_BLUE);
    
    void render();
    bool isDone() const;
};

#endif // hifi_OctreeFade_h
