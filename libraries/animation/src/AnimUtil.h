//
//  AnimUtil.h
//
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimUtil_h
#define hifi_AnimUtil_h

#include "AnimNode.h"

// TODO: use restrict keyword
// TODO: excellent candidate for simd vectorization.

// this is where the magic happens
void blend(size_t numPoses, const AnimPose* a, const AnimPose* b, float alpha, AnimPose* result);

#endif


