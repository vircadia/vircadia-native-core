//
//  VoxelNodeBag.cpp
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

#include "VoxelNodeBag.h"
#include <OctalCode.h>

VoxelNodeBag::~VoxelNodeBag() {
    if (_bagElements) {
        delete[] _bagElements;
    }
}

const int GROW_BAG_BY = 500;

// put a node into the bag
void VoxelNodeBag::insert(VoxelNode* node) {

printf("VoxelNodeBag::insert(node) node=");
node->printDebugDetails("");

    // Search for where we should live in the bag (sorted)
    // Note: change this to binary search... instead of linear!
    int insertAt = _elementsInUse;
    for (int i = 0; i < _elementsInUse; i++) {
    
        // compare the newNode to the elements already in the bag
        OctalTreeDepth comparison = compareOctalCodes(_bagElements[i]->octalCode,node->octalCode);
        
        // If we found a code in the bag that matches, then just return, since the element is already in the bag.
        if (comparison == EXACT_MATCH) {
            return; // exit early!!
        } 
        
        // if we found a node "greater than" the inserted node, then
        // we want to insert our node here.
        if (comparison == GREATER_THAN) {
            insertAt = i;
            break;
        }
    }
    // at this point, inserAt will be the location we want to insert at.
    
    // If we don't have room in our bag, then grow the bag
    if (_sizeOfElementsArray < _elementsInUse+1) {
        VoxelNode** oldBag = _bagElements;
        _bagElements = new VoxelNode*[_sizeOfElementsArray + GROW_BAG_BY];
        _sizeOfElementsArray += GROW_BAG_BY;
        
        // If we had an old bag...
        if (oldBag) {
            // copy old elements into the new bag, but leave a space where we need to
            // insert the new node
            memcpy(_bagElements, oldBag, insertAt);
            memcpy(&_bagElements[insertAt+1], &oldBag[insertAt], (_elementsInUse-insertAt));
        }
    } else {
        // move existing elements further back in the bag array, leave a space where we need to
        // insert the new node
        memcpy(&_bagElements[insertAt+1], &_bagElements[insertAt], (_elementsInUse-insertAt));
    }
    _bagElements[insertAt] = node;
    _elementsInUse++;
}
 
// pull a node out of the bag (could come in any order)
VoxelNode* VoxelNodeBag::extract() {
    // pull the last node out, and shrink our list...
    if (_elementsInUse) {
        
        // get the last element
        VoxelNode* node = _bagElements[_elementsInUse-1];
        
        // reduce the count
        _elementsInUse--;

        // debug!!
        printf("VoxelNodeBag::extract() node=");
        node->printDebugDetails("");

        return node;
    }
    return NULL;
}
