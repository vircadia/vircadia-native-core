//
//  AbstractAudioInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__AbstractAudioInterface__
#define __hifi__AbstractAudioInterface__

class AbstractAudioInterface {
public:
    virtual void startCollisionSound(float magnitude, float frequency, float noise, float duration, bool flashScreen) = 0;
    virtual void startDrumSound(float volume, float frequency, float duration, float decay) = 0;
};

#endif /* defined(__hifi__AbstractAudioInterface__) */