//
//  RemapIDOperator.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-12-6.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "EntityTree.h"
#include "RemapIDOperator.h"

QUuid RemapIDOperator::remap(const QUuid& oldID) {
    if (oldID.isNull()) {
        return oldID;
    }
    if (!_oldToNew.contains(oldID)) {
        _oldToNew[oldID] = QUuid::createUuid();
    }
    return _oldToNew[oldID];
}

bool RemapIDOperator::postRecursion(OctreeElementPointer element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    entityTreeElement->forEachEntity([&](EntityItemPointer entityItem) {
        entityItem->setID(remap(entityItem->getID()));
        entityItem->setParentID(remap(entityItem->getParentID()));
    });
    return true;
}
