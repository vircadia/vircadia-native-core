//
//  UpdateEntityOperator.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/11/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UpdateEntityOperator_h
#define hifi_UpdateEntityOperator_h

class UpdateEntityOperator : public RecurseOctreeOperator {
public:
    UpdateEntityOperator(EntityTreePointer tree, EntityTreeElementPointer containingElement,
                         EntityItemPointer existingEntity, const EntityItemProperties& properties);
    ~UpdateEntityOperator();

    virtual bool preRecursion(OctreeElementPointer element);
    virtual bool postRecursion(OctreeElementPointer element);
    virtual OctreeElementPointer possiblyCreateChildAt(OctreeElementPointer element, int childIndex);
private:
    EntityTreePointer _tree;
    EntityItemPointer _existingEntity;
    EntityTreeElementPointer _containingElement;
    AACube _containingElementCube; // we temporarily store our cube here in case we need to delete the containing element
    EntityItemProperties _properties;
    EntityItemID _entityItemID;
    bool _foundOld;
    bool _foundNew;
    bool _removeOld;
    bool _dontMove;
    quint64 _changeTime;

    AACube _oldEntityCube;
    AACube _newEntityCube;

    AABox _oldEntityBox; // clamped to domain
    AABox _newEntityBox; // clamped to domain

    bool subTreeContainsOldEntity(OctreeElementPointer element);
    bool subTreeContainsNewEntity(OctreeElementPointer element);

    bool _wantDebug;
};

#endif // hifi_UpdateEntityOperator_h
