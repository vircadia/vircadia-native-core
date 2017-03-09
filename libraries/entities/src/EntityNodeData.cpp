//
//  EntityNodeData.cpp
//  libraries/entities/src
//
//  Created by Stephen Birarda on 2/15/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityNodeData.h"

bool EntityNodeData::insertFlaggedExtraEntity(const QUuid& filteredEntityID, const QUuid& extraEntityID) {
    _flaggedExtraEntities[filteredEntityID].insert(extraEntityID);
    return !_previousFlaggedExtraEntities[filteredEntityID].contains(extraEntityID);
}

bool EntityNodeData::isEntityFlaggedAsExtra(const QUuid& entityID) const {

    // enumerate each of the sets for the entities that matched our filter
    // and immediately return true if any of them contain this entity ID
    foreach(QSet<QUuid> entitySet, _flaggedExtraEntities) {
        if (entitySet.contains(entityID)) {
            return true;
        }
    }

    return false;
}
