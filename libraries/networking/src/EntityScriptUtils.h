//
//  EntityScriptUtils.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2017/01/13
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityScriptUtils_h
#define hifi_EntityScriptUtils_h
#include <QMetaEnum>

class EntityScriptStatus_ : public QObject {
    Q_OBJECT
public:
    enum EntityScriptStatus {
        PENDING,
        LOADING,
        ERROR_LOADING_SCRIPT,
        ERROR_RUNNING_SCRIPT,
        RUNNING,
        UNLOADED
    };
    Q_ENUM(EntityScriptStatus)
    static QString valueToKey(EntityScriptStatus status) {
        return QMetaEnum::fromType<EntityScriptStatus>().valueToKey(status);
    }
};
using EntityScriptStatus = EntityScriptStatus_::EntityScriptStatus;
#endif // hifi_EntityScriptUtils_h