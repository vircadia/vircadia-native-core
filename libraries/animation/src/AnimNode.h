//
//  AnimInterface.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimNode_h
#define hifi_AnimNode_h

class AnimNode {
public:
    virtual float getEnd() const = 0;
    virtual const AnimPose& evaluate(float t) = 0;
};

#endif // hifi_AnimNode_h
