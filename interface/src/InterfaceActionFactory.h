//
//  InterfaceActionFactory.cpp
//  interface/src/
//
//  Created by Seth Alves on 2015-6-10
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InterfaceActionFactory_h
#define hifi_InterfaceActionFactory_h

#include "EntityActionFactoryInterface.h"

class InterfaceActionFactory : public EntityActionFactoryInterface {
public:
    InterfaceActionFactory() : EntityActionFactoryInterface() { }
    virtual ~InterfaceActionFactory() { }
    virtual EntityActionPointer factory(EntityActionType type,
                                        const QUuid& id,
                                        EntityItemPointer ownerEntity,
                                        QVariantMap arguments) override;
    virtual EntityActionPointer factoryBA(EntityItemPointer ownerEntity,
                                          QByteArray data) override;
};

#endif // hifi_InterfaceActionFactory_h
