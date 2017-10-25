//
//  Created by Sam Gondelman 10/20/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "PickScriptingInterface.h"

#include <QVariant>
#include "GLMHelpers.h"

#include <pointers/PickManager.h>

#include "StaticRayPick.h"
#include "JointRayPick.h"
#include "MouseRayPick.h"

#include <pointers/Pick.h>
#include <ScriptEngine.h>

QUuid PickScriptingInterface::createPick(const PickQuery::PickType type, const QVariant& properties) {
    switch (type) {
        case PickQuery::PickType::Ray:
            return createRayPick(properties);
        default:
            return QUuid();
    }
}

QUuid PickScriptingInterface::createRayPick(const QVariant& properties) {
    QVariantMap propMap = properties.toMap();

    bool enabled = false;
    if (propMap["enabled"].isValid()) {
        enabled = propMap["enabled"].toBool();
    }

    PickFilter filter = PickFilter();
    if (propMap["filter"].isValid()) {
        filter = PickFilter(propMap["filter"].toUInt());
    }

    float maxDistance = 0.0f;
    if (propMap["maxDistance"].isValid()) {
        maxDistance = propMap["maxDistance"].toFloat();
    }

    if (propMap["joint"].isValid()) {
        std::string jointName = propMap["joint"].toString().toStdString();

        if (jointName != "Mouse") {
            // x = upward, y = forward, z = lateral
            glm::vec3 posOffset = Vectors::ZERO;
            if (propMap["posOffset"].isValid()) {
                posOffset = vec3FromVariant(propMap["posOffset"]);
            }

            glm::vec3 dirOffset = Vectors::UP;
            if (propMap["dirOffset"].isValid()) {
                dirOffset = vec3FromVariant(propMap["dirOffset"]);
            }

            return DependencyManager::get<PickManager>()->addPick(PickQuery::Ray, std::make_shared<JointRayPick>(jointName, posOffset, dirOffset, filter, maxDistance, enabled));

        } else {
            return DependencyManager::get<PickManager>()->addPick(PickQuery::Ray, std::make_shared<MouseRayPick>(filter, maxDistance, enabled));
        }
    } else if (propMap["position"].isValid()) {
        glm::vec3 position = vec3FromVariant(propMap["position"]);

        glm::vec3 direction = -Vectors::UP;
        if (propMap["direction"].isValid()) {
            direction = vec3FromVariant(propMap["direction"]);
        }

        return DependencyManager::get<PickManager>()->addPick(PickQuery::Ray, std::make_shared<StaticRayPick>(position, direction, filter, maxDistance, enabled));
    }

    return QUuid();
}

void PickScriptingInterface::enablePick(const QUuid& uid) {
    DependencyManager::get<PickManager>()->enablePick(uid);
}

void PickScriptingInterface::disablePick(const QUuid& uid) {
    DependencyManager::get<PickManager>()->disablePick(uid);
}

void PickScriptingInterface::removePick(const QUuid& uid) {
    DependencyManager::get<PickManager>()->removePick(uid);
}

QVariantMap PickScriptingInterface::getPrevPickResult(const QUuid& uid) {
    QVariantMap result;
    auto pickResult = DependencyManager::get<PickManager>()->getPrevPickResult(uid);
    if (pickResult) {
        result = pickResult->toVariantMap();
    }
    return result;
}

void PickScriptingInterface::setPrecisionPicking(const QUuid& uid, const bool precisionPicking) {
    DependencyManager::get<PickManager>()->setPrecisionPicking(uid, precisionPicking);
}

void PickScriptingInterface::setIgnoreItems(const QUuid& uid, const QScriptValue& ignoreItems) {
    DependencyManager::get<PickManager>()->setIgnoreItems(uid, qVectorQUuidFromScriptValue(ignoreItems));
}

void PickScriptingInterface::setIncludeItems(const QUuid& uid, const QScriptValue& includeItems) {
    DependencyManager::get<PickManager>()->setIncludeItems(uid, qVectorQUuidFromScriptValue(includeItems));
}

QScriptValue pickTypesToScriptValue(QScriptEngine* engine, const PickQuery::PickType& pickType) {
    return pickType;
}

void pickTypesFromScriptValue(const QScriptValue& object, PickQuery::PickType& pickType) {
    pickType = static_cast<PickQuery::PickType>(object.toUInt16());
}

void PickScriptingInterface::registerMetaTypes(QScriptEngine* engine) {
    QScriptValue pickTypes = engine->newObject();
    auto metaEnum = QMetaEnum::fromType<PickQuery::PickType>();
    for (int i = 0; i < PickQuery::PickType::NUM_PICK_TYPES; ++i) {
        pickTypes.setProperty(metaEnum.key(i), metaEnum.value(i));
    }
    engine->globalObject().setProperty("PickType", pickTypes);

    qScriptRegisterMetaType(engine, pickTypesToScriptValue, pickTypesFromScriptValue);
}