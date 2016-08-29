//
//  InterfaceParentFinder.h
//  interface/src/
//
//  Created by Seth Alves on 2015-10-21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InterfaceParentFinder_h
#define hifi_InterfaceParentFinder_h

#include <memory>
#include <QUuid>

#include <SpatialParentFinder.h>

class InterfaceParentFinder : public SpatialParentFinder {
public:
    InterfaceParentFinder() { }
    virtual ~InterfaceParentFinder() { }
    virtual SpatiallyNestableWeakPointer find(QUuid parentID, bool& success,
                                              SpatialParentTree* entityTree = nullptr) const override;
};

#endif // hifi_InterfaceParentFinder_h
