//
//  OctreeElementBag.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 4/25/2013
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  This class is used by the VoxelTree:encodeTreeBitstream() functions to store extra nodes that need to be sent
//  it's a generic bag style storage mechanism. But It has the property that you can't put the same node into the bag
//  more than once (in other words, it de-dupes automatically), also, it supports collapsing it's several peer nodes
//  into a parent node in cases where you add enough peers that it makes more sense to just add the parent.
//

#ifndef __hifi__OctreeElementBag__
#define __hifi__OctreeElementBag__

#include "OctreeElement.h"

class OctreeElementBag : public OctreeElementDeleteHook {

public:
    OctreeElementBag();
    ~OctreeElementBag();
    
    void insert(OctreeElement* element); // put a element into the bag
    OctreeElement* extract(); // pull a element out of the bag (could come in any order)
    bool contains(OctreeElement* element); // is this element in the bag?
    void remove(OctreeElement* element); // remove a specific element from the bag
    
    bool isEmpty() const { return (_elementsInUse == 0); }
    int count() const { return _elementsInUse; }

    void deleteAll();
    virtual void elementDeleted(OctreeElement* element);

private:
    
    OctreeElement** _bagElements;
    int _elementsInUse;
    int _sizeOfElementsArray;
    //int _hookID;
};

#endif /* defined(__hifi__OctreeElementBag__) */
