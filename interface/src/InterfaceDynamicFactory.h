//
//  InterfaceDynamicFactory.cpp
//  interface/src/
//
//  Created by Seth Alves on 2015-6-10
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_InterfaceDynamicFactory_h
#define hifi_InterfaceDynamicFactory_h

#include "EntityDynamicFactoryInterface.h"

class InterfaceDynamicFactory : public EntityDynamicFactoryInterface {
public:
    InterfaceDynamicFactory() : EntityDynamicFactoryInterface() { }
    virtual ~InterfaceDynamicFactory() { }
    virtual EntityDynamicPointer factory(EntityDynamicType type,
                                        const QUuid& id,
                                        EntityItemPointer ownerEntity,
                                        QVariantMap arguments) override;
    virtual EntityDynamicPointer factoryBA(EntityItemPointer ownerEntity,
                                          QByteArray data) override;
};

#endif // hifi_InterfaceDynamicFactory_h
