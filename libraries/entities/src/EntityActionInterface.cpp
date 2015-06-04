//
//  EntityItem.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-4
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityItem.h"

#include "EntityActionInterface.h"


EntityActionType EntityActionInterface::actionTypeFromString(QString actionTypeString) {
    QString normalizedActionTypeString = actionTypeString.toLower().remove('-').remove('_');
    if (normalizedActionTypeString == "none") {
        return ACTION_TYPE_NONE;
    }
    if (normalizedActionTypeString == "pulltopoint") {
        return ACTION_TYPE_PULL_TO_POINT;
    }

    qDebug() << "Warning -- EntityActionInterface::actionTypeFromString got unknown action-type name" << actionTypeString;
    return ACTION_TYPE_NONE;
}

QString EntityActionInterface::actionTypeToString(EntityActionType actionType) {
    switch(actionType) {
        case ACTION_TYPE_NONE:
            return "none";
        case ACTION_TYPE_PULL_TO_POINT:
            return "pullToPoint";
    }
    assert(false);
    return "none";
}
