//
//  DeleteEntityOperator.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/11/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DeleteEntityOperator_h
#define hifi_DeleteEntityOperator_h

#include <QSet>

#include <AACube.h>

#include "EntityItem.h"

class EntityToDeleteDetails {
public:
    EntityItemPointer entity;
    AACube cube;
    EntityTreeElementPointer containingElement;
};

typedef QSet<EntityToDeleteDetails> RemovedEntities;

inline uint qHash(const EntityToDeleteDetails& a, uint seed) {
    return qHash(a.entity->getEntityItemID(), seed);
}

inline bool operator==(const EntityToDeleteDetails& a, const EntityToDeleteDetails& b) {
    return a.entity->getEntityItemID() == b.entity->getEntityItemID();
}

class DeleteEntityOperator : public RecurseOctreeOperator {
public:
    DeleteEntityOperator(EntityTreePointer tree);
    DeleteEntityOperator(EntityTreePointer tree, const EntityItemID& searchEntityID);
    ~DeleteEntityOperator();

    void addEntityIDToDeleteList(const EntityItemID& searchEntityID);
    void addEntityToDeleteList(const EntityItemPointer& entity);
    virtual bool preRecursion(const OctreeElementPointer& element) override;
    virtual bool postRecursion(const OctreeElementPointer& element) override;

    const RemovedEntities& getEntities() const { return _entitiesToDelete; }
private:
    EntityTreePointer _tree;
    RemovedEntities _entitiesToDelete;
    quint64 _changeTime;
    int _foundCount;
    int _lookingCount;
    bool subTreeContainsSomeEntitiesToDelete(const OctreeElementPointer& element);
};

#endif // hifi_DeleteEntityOperator_h
