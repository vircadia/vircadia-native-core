//
//  SpatialParentFinder.h
//  libraries/shared/src/
//
//  Created by Seth Alves on 2015-10-18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SpatialParentFinder_h
#define hifi_SpatialParentFinder_h

#include <QUuid>

#include "DependencyManager.h"

class SpatiallyNestable;
using SpatiallyNestableWeakPointer = std::weak_ptr<SpatiallyNestable>;
using SpatiallyNestablePointer = std::shared_ptr<SpatiallyNestable>;
class SpatialParentTree {
public:
    virtual SpatiallyNestablePointer findByID(const QUuid& id) { return nullptr; }
};
class SpatialParentFinder : public Dependency {



// This interface is used to turn a QUuid into a pointer to a "parent" -- something that children can
// be spatially relative to.  At this point, this means either an EntityItem or an Avatar.


public:
    SpatialParentFinder() { }
    virtual ~SpatialParentFinder() { }

    virtual SpatiallyNestableWeakPointer find(QUuid parentID, bool& success, SpatialParentTree* entityTree = nullptr) const = 0;
};

#endif // hifi_SpatialParentFinder_h
