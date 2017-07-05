//
//  SimpleMeshProxy.cpp
//  libraries/model-networking/src/model-networking/
//
//  Created by Seth Alves on 2017-3-22.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SimpleMeshProxy.h"

#include <model/Geometry.h>

MeshPointer SimpleMeshProxy::getMeshPointer() const {
    return _mesh;
}

int SimpleMeshProxy::getNumVertices() const {
    return (int)_mesh->getNumVertices();
}

glm::vec3 SimpleMeshProxy::getPos3(int index) const {
    return _mesh->getPos3(index);
}

