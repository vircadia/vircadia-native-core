//
//  OctreeElementBag.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 4/25/2013.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
    
    bool isEmpty() const { return _bagElements.isEmpty(); }
    int count() const { return _bagElements.size(); }

    void deleteAll();
    virtual void elementDeleted(OctreeElement* element);

private:
    QSet<OctreeElement*> _bagElements;
};

#endif /* defined(__hifi__OctreeElementBag__) */
