//
//  RemapIDOperator.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-12-6.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RemapIDOperator_h
#define hifi_RemapIDOperator_h

#include "Octree.h"

// this will change all the IDs in an EntityTree.  Parent/Child relationships are maintained.

class RemapIDOperator : public RecurseOctreeOperator {
public:
    RemapIDOperator() : RecurseOctreeOperator() {}
    ~RemapIDOperator() {}
    virtual bool preRecursion(OctreeElementPointer element) { return true; }
    virtual bool postRecursion(OctreeElementPointer element);
private:
    QUuid remap(const QUuid& oldID);
    QHash<QUuid, QUuid> _oldToNew;
};

#endif // hifi_RemapIDOperator_h
