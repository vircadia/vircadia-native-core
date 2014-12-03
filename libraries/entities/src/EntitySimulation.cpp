//
//  EntitySimulation.cpp
//  libraries/entities/src
//
//  Created by Andrew Meadows on 2014.11.24
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntitySimulation.h"

void EntitySimulation::setEntityTree(EntityTree* tree) {
    if (_entityTree && _entityTree != tree) {
        clearEntities();
    }
    _entityTree = tree;
}

