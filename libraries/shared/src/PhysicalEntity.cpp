//
//  PhysicalEntity.cpp
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.06.11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PhysicalEntity.h"
#include "Shape.h"

// static 
void PhysicalEntity::setShapeBackPointers(const QVector<Shape*>& shapes, PhysicalEntity* entity) {
    for (int i = 0; i < shapes.size(); i++) {
        Shape* shape = shapes[i];
        if (shape) {
            shape->setEntity(entity);
        }
    }
}

