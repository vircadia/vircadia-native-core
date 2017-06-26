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

#include "EntitiesLogging.h"
#include "EntityItem.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"

class UpdateEntityOperator : public RecurseOctreeOperator {
public:
    UpdateEntityOperator(EntityTreePointer tree, EntityTreeElementPointer containingElement,
                         EntityItemPointer existingEntity, const AACube newQueryAACube);

    ~UpdateEntityOperator();

    virtual bool preRecursion(const OctreeElementPointer& element) override;
    virtual bool postRecursion(const OctreeElementPointer& element) override;
    virtual OctreeElementPointer possiblyCreateChildAt(const OctreeElementPointer& element, int childIndex) override;
private:
    EntityTreePointer _tree;
    EntityItemPointer _existingEntity;
    EntityTreeElementPointer _containingElement;
    AACube _containingElementCube; // we temporarily store our cube here in case we need to delete the containing element
    EntityItemID _entityItemID;
    bool _foundOld;
    bool _foundNew;
    bool _removeOld;
    quint64 _changeTime;

    AACube _oldEntityCube;
    AACube _newEntityCube;

    AABox _oldEntityBox; // clamped to domain
    AABox _newEntityBox; // clamped to domain

    bool subTreeContainsOldEntity(const OctreeElementPointer& element);
    bool subTreeContainsNewEntity(const OctreeElementPointer& element);

    bool _wantDebug;
};

#endif // hifi_UpdateEntityOperator_h
