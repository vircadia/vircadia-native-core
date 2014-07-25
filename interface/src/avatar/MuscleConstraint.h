//
//  MuscleConstraint.h
//  interface/src/avatar
//
//  Created by Andrew Meadows 2014.07.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MuscleConstraint_h
#define hifi_MuscleConstraint_h

#include <Constraint.h>

// MuscleConstraint is a simple constraint that pushes the child toward an offset relative to the parent.
// It does NOT push the parent.

const float MAX_MUSCLE_STRENGTH = 0.5f;

class MuscleConstraint : public Constraint {
public:
    MuscleConstraint(VerletPoint* parent, VerletPoint* child);
    MuscleConstraint(const MuscleConstraint& other);
    float enforce();

    void setIndices(int parent, int child);
    int getParentIndex() const { return _parentIndex; }
    int getChildIndex() const { return _childndex; }
    void setChildOffset(const glm::vec3& offset) { _childOffset = offset; }
    void setStrength(float strength) { _strength = glm::clamp(strength, 0.0f, MAX_MUSCLE_STRENGTH); }
    float getStrength() const { return _strength; }
private:
    VerletPoint* _rootPoint;
    VerletPoint* _childoint;
    int _parentIndex;
    int _childndex;
    glm::vec3 _childOffset;
    float _strength;    // a value in range [0,MAX_MUSCLE_STRENGTH]
};

#endif // hifi_MuscleConstraint_h
