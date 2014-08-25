//
//  MovingEntitiesOperator.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/11/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MovingEntitiesOperator_h
#define hifi_MovingEntitiesOperator_h

class EntityToMoveDetails {
public:
    EntityItem* entity;
    AACube oldCube;
    AACube newCube;
    EntityTreeElement* oldContainingElement;
    bool oldFound;
    bool newFound;
};

inline uint qHash(const EntityToMoveDetails& a, uint seed) {
    return qHash(a.entity->getEntityItemID(), seed);
}

inline bool operator==(const EntityToMoveDetails& a, const EntityToMoveDetails& b) {
    return a.entity->getEntityItemID() == b.entity->getEntityItemID();
}

class MovingEntitiesOperator : public RecurseOctreeOperator {
public:
    MovingEntitiesOperator(EntityTree* tree);
    void addEntityToMoveList(EntityItem* entity, const AACube& oldCube, const AACube& newCube);
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
    virtual OctreeElement* PossiblyCreateChildAt(OctreeElement* element, int childIndex);
private:
    EntityTree* _tree;
    QSet<EntityToMoveDetails> _entitiesToMove;
    quint64 _changeTime;
    int _foundOldCount;
    int _foundNewCount;
    int _lookingCount;
    bool shouldRecurseSubTree(OctreeElement* element);
};

#endif // hifi_MovingEntitiesOperator_h
