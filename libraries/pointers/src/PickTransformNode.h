//
//  Created by Sabrina Shanman 2018/08/22
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PickTransformNode_h
#define hifi_PickTransformNode_h

#include "TransformNode.h"

// TODO: Remove this class when Picks are converted to SpatiallyNestables
class PickTransformNode : public TransformNode {
public:
    PickTransformNode(unsigned int uid);
    Transform getTransform() override;
    QVariantMap toVariantMap() const override;

protected:
    unsigned int _uid;
};

#endif // hifi_PickTransformNode_h
