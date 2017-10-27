//
//  DirtyOctreeElementOperator.h
//  libraries/entities/src
//
//  Created by Andrew Meawdows 2016.02.04
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DirtyOctreeElementOperator_h
#define hifi_DirtyOctreeElementOperator_h

#include "Octree.h"

class DirtyOctreeElementOperator : public RecurseOctreeOperator {
public:
    DirtyOctreeElementOperator(const OctreeElementPointer& element);

    ~DirtyOctreeElementOperator() {}

    virtual bool preRecursion(const OctreeElementPointer& element) override;
    virtual bool postRecursion(const OctreeElementPointer& element) override;
private:
    glm::vec3 _point;
    OctreeElementPointer _element;
};

#endif // hifi_DirtyOctreeElementOperator_h
