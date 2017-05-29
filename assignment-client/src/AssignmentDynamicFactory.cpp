//
//  AssignmentDynamcFactory.cpp
//  assignment-client/src/
//
//  Created by Seth Alves on 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssignmentDynamicFactory.h"


EntityDynamicPointer assignmentDynamicFactory(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity) {
    return EntityDynamicPointer(new AssignmentDynamic(type, id, ownerEntity));
}

EntityDynamicPointer AssignmentDynamicFactory::factory(EntityDynamicType type,
                                                     const QUuid& id,
                                                     EntityItemPointer ownerEntity,
                                                     QVariantMap arguments) {
    EntityDynamicPointer dynamic = assignmentDynamicFactory(type, id, ownerEntity);
    if (dynamic) {
        bool ok = dynamic->updateArguments(arguments);
        if (ok) {
            return dynamic;
        }
    }
    return nullptr;
}


EntityDynamicPointer AssignmentDynamicFactory::factoryBA(EntityItemPointer ownerEntity, QByteArray data) {
    QDataStream serializedDynamicDataStream(data);
    EntityDynamicType type;
    QUuid id;

    serializedDynamicDataStream >> type;
    serializedDynamicDataStream >> id;

    EntityDynamicPointer dynamic = assignmentDynamicFactory(type, id, ownerEntity);

    if (dynamic) {
        dynamic->deserialize(data);
    }
    return dynamic;
}
