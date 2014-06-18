//
//  VerletSphereShape.h
//  libraries/shared/src
//
//  Created by Andrew Meadows on 2014.06.16
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VerletSphereShape_h
#define hifi_VerletSphereShape_h

#include "SphereShape.h"

// The VerletSphereShape is similar to a regular SphereShape, except it keeps a pointer
// to its center which is owned by some other data structure.  This allows it to 
// participate in a verlet integration engine.
class VerletSphereShape : public SphereShape {
public:
    VerletSphereShape(glm::vec3* centerPoint);

    VerletSphereShape(float radius, glm::vec3* centerPoint);

    void setTranslation(const glm::vec3& position);
    glm::vec3 getTranslation();

protected:
    // NOTE: VerletSphereShape does NOT own its _point
    glm::vec3* _point;
};

#endif // hifi_VerletSphereShape_h
