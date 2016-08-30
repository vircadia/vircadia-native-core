//
//  AssignmentParentFinder.h
//  interface/src/entities
//
//  Created by Seth Alves on 2015-10-21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentParentFinder_h
#define hifi_AssignmentParentFinder_h

#include <memory>
#include <QUuid>

#include <EntityTree.h>
#include <SpatialParentFinder.h>

// This interface is used to turn a QUuid into a pointer to a "parent" -- something that children can
// be spatially relative to.  At this point, this means either an EntityItem or an Avatar.

class AssignmentParentFinder : public SpatialParentFinder {
public:
    AssignmentParentFinder(EntityTreePointer tree) : _tree(tree) { }
    virtual ~AssignmentParentFinder() { }
    virtual SpatiallyNestableWeakPointer find(QUuid parentID, bool& success,
                                              SpatialParentTree* entityTree = nullptr) const override;

protected:
    EntityTreePointer _tree;
};

#endif // hifi_AssignmentParentFinder_h
