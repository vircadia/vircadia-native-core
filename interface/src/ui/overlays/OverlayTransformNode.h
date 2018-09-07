//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_OverlayTransformNode_h
#define hifi_OverlayTransformNode_h

#include "NestableTransformNode.h"

#include "Base3DOverlay.h"

// For 3D overlays only
class OverlayTransformNode : public BaseNestableTransformNode<Base3DOverlay> {
public:
    OverlayTransformNode(std::weak_ptr<Base3DOverlay> spatiallyNestable, int jointIndex) : BaseNestableTransformNode(spatiallyNestable, jointIndex) {};
};

#endif // hifi_OverlayTransformNode_h