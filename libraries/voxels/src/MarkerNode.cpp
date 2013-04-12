//
//  MarkerNode.cpp
//  hifi
//
//  Created by Stephen Birarda on 3/26/13.
//
//

#include "MarkerNode.h"

MarkerNode::MarkerNode() {
    for (int i = 0; i < 8; i++) {
        children[i] = NULL;
    }
    
    childrenVisitedMask = 0;
}

MarkerNode::~MarkerNode() {
    for (int i = 0; i < 8; i++) {
        delete children[i];
    }
}

MarkerNode::MarkerNode(const MarkerNode &otherMarkerNode) {
    childrenVisitedMask = otherMarkerNode.childrenVisitedMask;
    
    // recursively copy the children marker nodes
    for (int i = 0; i < 8; i++) {
        if (children[i] != NULL) {
            children[i] = new MarkerNode(*otherMarkerNode.children[i]);
        }
    }
}