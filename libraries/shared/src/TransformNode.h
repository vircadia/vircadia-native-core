//
//  Created by Sabrina Shanman 2018/08/14
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_TransformNode_h
#define hifi_TransformNode_h

#include <QVariant>

#include "Transform.h"

class TransformNode {
public:
    virtual ~TransformNode() = default;

    virtual Transform getTransform() = 0;
    virtual QVariantMap toVariantMap() const = 0;
};

#endif // hifi_TransformNode_h
