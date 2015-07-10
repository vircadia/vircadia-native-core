//
//  EntityActionFactoryInterface.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityActionFactoryInterface_h
#define hifi_EntityActionFactoryInterface_h

#include <DependencyManager.h>

#include "EntityActionInterface.h"

class EntityActionFactoryInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

 public:
    EntityActionFactoryInterface() { }
    virtual ~EntityActionFactoryInterface() { }
    virtual EntityActionPointer factory(EntityActionType type,
                                        const QUuid& id,
                                        EntityItemPointer ownerEntity,
                                        QVariantMap arguments) { assert(false); return nullptr; }
    virtual EntityActionPointer factoryBA(EntityItemPointer ownerEntity,
                                          QByteArray data) { assert(false); return nullptr; }
};

#endif // hifi_EntityActionFactoryInterface_h
