//
//  AddEntityOperator.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 8/11/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AddEntityOperator_h
#define hifi_AddEntityOperator_h

class AddEntityOperator : public RecurseOctreeOperator {
public:
    AddEntityOperator(EntityTree* tree, EntityItem* newEntity);
                            
    virtual bool PreRecursion(OctreeElement* element);
    virtual bool PostRecursion(OctreeElement* element);
    virtual OctreeElement* PossiblyCreateChildAt(OctreeElement* element, int childIndex);
private:
    EntityTree* _tree;
    EntityItem* _newEntity;
    bool _foundNew;
    quint64 _changeTime;

    AABox _newEntityBox;
};


#endif // hifi_AddEntityOperator_h
