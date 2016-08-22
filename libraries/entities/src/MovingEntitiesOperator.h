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
    EntityItemPointer entity;
    AACube oldCube; // meters
    AACube newCube; // meters
    AABox newCubeClamped; // meters
    EntityTreeElementPointer oldContainingElement;
    AACube oldContainingElementCube; // meters
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
    MovingEntitiesOperator(EntityTreePointer tree);
    ~MovingEntitiesOperator();

    void addEntityToMoveList(EntityItemPointer entity, const AACube& newCube);
    virtual bool preRecursion(OctreeElementPointer element) override;
    virtual bool postRecursion(OctreeElementPointer element) override;
    virtual OctreeElementPointer possiblyCreateChildAt(OctreeElementPointer element, int childIndex) override;
    bool hasMovingEntities() const { return _entitiesToMove.size() > 0; }
private:
    EntityTreePointer _tree;
    QSet<EntityToMoveDetails> _entitiesToMove;
    quint64 _changeTime;
    int _foundOldCount;
    int _foundNewCount;
    int _lookingCount;
    bool shouldRecurseSubTree(OctreeElementPointer element);
    
    bool _wantDebug;
};

#endif // hifi_MovingEntitiesOperator_h
