//
//  RecurseOctreeToMapOperator.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 3/16/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RecurseOctreeToMapOperator.h"

#include "EntityItemProperties.h"

RecurseOctreeToMapOperator::RecurseOctreeToMapOperator(QVariantMap& map,
                                                       OctreeElementPointer top,
                                                       QScriptEngine* engine,
                                                       bool skipDefaultValues,
                                                       bool skipThoseWithBadParents) :
        RecurseOctreeOperator(),
        _map(map),
        _top(top),
        _engine(engine),
        _skipDefaultValues(skipDefaultValues),
        _skipThoseWithBadParents(skipThoseWithBadParents)
{
    // if some element "top" was given, only save information for that element and its children.
    if (_top) {
        _withinTop = false;
    } else {
        // top was NULL, export entire tree.
        _withinTop = true;
    }
};

bool RecurseOctreeToMapOperator::preRecursion(OctreeElementPointer element) {
    if (element == _top) {
        _withinTop = true;
    }
    return true;
}

bool RecurseOctreeToMapOperator::postRecursion(OctreeElementPointer element) {

    EntityItemProperties defaultProperties;

    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);
    QVariantList entitiesQList = qvariant_cast<QVariantList>(_map["Entities"]);

    entityTreeElement->forEachEntity([&](EntityItemPointer entityItem) {
        if (_skipThoseWithBadParents && !entityItem->isParentIDValid()) {
            return;  // we weren't able to resolve a parent from _parentID, so don't save this entity.
        }

        EntityItemProperties properties = entityItem->getProperties();
        QScriptValue qScriptValues;
        if (_skipDefaultValues) {
            qScriptValues = EntityItemNonDefaultPropertiesToScriptValue(_engine, properties);
        } else {
            qScriptValues = EntityItemPropertiesToScriptValue(_engine, properties);
        }
        entitiesQList << qScriptValues.toVariant();
    });

    _map["Entities"] = entitiesQList;
    if (element == _top) {
        _withinTop = false;
    }
    return true;
}
