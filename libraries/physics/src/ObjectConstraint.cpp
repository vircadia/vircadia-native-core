//
//  ObjectConstraint.cpp
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitySimulation.h"

#include "ObjectConstraint.h"

#include "PhysicsLogging.h"

ObjectConstraint::ObjectConstraint(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) :
    ObjectDynamic(type, id, ownerEntity)
{
}

void ObjectConstraint::invalidate() {
    _constraint = nullptr;
}
