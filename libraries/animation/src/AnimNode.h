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

typedef float AnimPose;

class AnimNode {
public:
    virtual ~AnimNode() {}

    virtual const AnimPose& evaluate(float dt) = 0;
};

#endif // hifi_AnimNode_h
