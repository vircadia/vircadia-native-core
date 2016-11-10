//
//  AssignmentActionFactory.cpp
//  assignment-client/src/
//
//  Created by Seth Alves on 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentActionFactory_h
#define hifi_AssignmentActionFactory_h

#include "EntityActionFactoryInterface.h"
#include "AssignmentAction.h"

class AssignmentActionFactory : public EntityActionFactoryInterface {
public:
    AssignmentActionFactory() : EntityActionFactoryInterface() { }
    virtual ~AssignmentActionFactory() { }
    virtual EntityActionPointer factory(EntityActionType type,
                                        const QUuid& id,
                                        EntityItemPointer ownerEntity,
                                        QVariantMap arguments) override;
    virtual EntityActionPointer factoryBA(EntityItemPointer ownerEntity, QByteArray data) override;
};

#endif // hifi_AssignmentActionFactory_h
