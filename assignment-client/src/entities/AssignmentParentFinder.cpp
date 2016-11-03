//
//  AssignmentParentFinder.cpp
//  assignment-client/src/entities
//
//  Created by Seth Alves on 2015-10-22
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssignmentParentFinder.h"

SpatiallyNestableWeakPointer AssignmentParentFinder::find(QUuid parentID, bool& success, SpatialParentTree* entityTree) const {
    SpatiallyNestableWeakPointer parent;

    if (parentID.isNull()) {
        success = true;
        return parent;
    }

    // search entities
    if (entityTree) {
        parent = entityTree->findByID(parentID);
    } else {
        parent = _tree->findEntityByEntityItemID(parentID);
    }
    if (parent.expired()) {
        success = false;
    } else {
        success = true;
    }
    return parent;
}
