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
// to its center which is owned by some other data structure (a verlet simulation system).  
// This makes it easier for the points to be moved around by constraints in the system
// as well as collisions with the shape, however it has some drawbacks:
//
// (1) The Shape::_translation data member is not used (wasted)
//
// (2) A VerletShape doesn't own the points that it uses, so you must be careful not to
//     leave dangling pointers around.

class VerletPoint;

class VerletSphereShape : public SphereShape {
public:
    VerletSphereShape(VerletPoint* point);

    VerletSphereShape(float radius, VerletPoint* centerPoint);

    // virtual overrides from Shape
    void setTranslation(const glm::vec3& position);
    const glm::vec3& getTranslation() const;
    float computeEffectiveMass(const glm::vec3& penetration, const glm::vec3& contactPoint);
    void accumulateDelta(float relativeMassFactor, const glm::vec3& penetration);
    void applyAccumulatedDelta();
    void getVerletPoints(QVector<VerletPoint*>& points);


protected:
    // NOTE: VerletSphereShape does NOT own its _point
    VerletPoint* _point;
};

#endif // hifi_VerletSphereShape_h
