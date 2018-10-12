//
//  RecurseOctreeToJSONOperator.cpp
//  libraries/entities/src
//
//  Created by Simon Walton on Oct 11, 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "RecurseOctreeToJSONOperator.h"

#include "EntityItemProperties.h"

RecurseOctreeToJSONOperator::RecurseOctreeToJSONOperator(const OctreeElementPointer& top, QScriptEngine* engine,
    QString jsonPrefix /* = QString() */)
    : _top(top)
    , _engine(engine)
    , _json(jsonPrefix)
{
    _toStringMethod = _engine->evaluate("(function() { return JSON.stringify(this, null, '    ') })");
}

bool RecurseOctreeToJSONOperator::postRecursion(const OctreeElementPointer& element) {
    EntityTreeElementPointer entityTreeElement = std::static_pointer_cast<EntityTreeElement>(element);

    entityTreeElement->forEachEntity([&](const EntityItemPointer& entity) { processEntity(entity); } );
    return true;
}

void RecurseOctreeToJSONOperator::processEntity(const EntityItemPointer& entity) {
    QScriptValue qScriptValues = EntityItemNonDefaultPropertiesToScriptValue(_engine, entity->getProperties());
    if (comma) {
        _json += ',';
    };
    comma = true;
    _json += "\n    ";

    // Override default toString():
    qScriptValues.setProperty("toString", _toStringMethod);
    QString jsonResult = qScriptValues.toString();
    //auto exceptionString2 = _engine->uncaughtException().toString();
    _json += jsonResult;
}
