//
//  Created by Sabrina Shanman 9/5/2018
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_OverlayTransformNode_h
#define hifi_OverlayTransformNode_h

#include "TransformNode.h"

#include "Base3DOverlay.h"

// For 3D overlays only
class OverlayTransformNode : public TransformNode {
public:
    OverlayTransformNode(std::weak_ptr<Base3DOverlay> overlay, int jointIndex);
    Transform getTransform() override;

protected:
    std::weak_ptr<Base3DOverlay> _overlay;
    int _jointIndex;
};

#endif // hifi_OverlayTransformNode_h