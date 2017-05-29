//
//  AssignmentDynamicFactory.cpp
//  assignment-client/src/
//
//  Created by Seth Alves on 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentDynamicFactory_h
#define hifi_AssignmentDynamicFactory_h

#include "EntityDynamicFactoryInterface.h"
#include "AssignmentDynamic.h"

class AssignmentDynamicFactory : public EntityDynamicFactoryInterface {
public:
    AssignmentDynamicFactory() : EntityDynamicFactoryInterface() { }
    virtual ~AssignmentDynamicFactory() { }
    virtual EntityDynamicPointer factory(EntityDynamicType type,
                                        const QUuid& id,
                                        EntityItemPointer ownerEntity,
                                        QVariantMap arguments) override;
    virtual EntityDynamicPointer factoryBA(EntityItemPointer ownerEntity, QByteArray data) override;
};

#endif // hifi_AssignmentDynamicFactory_h
