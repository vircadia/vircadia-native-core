//
//  MarkerNode.h
//  hifi
//
//  Created by Stephen Birarda on 3/26/13.
//
//

#ifndef __hifi__MarkerNode__
#define __hifi__MarkerNode__

#include <iostream>

class MarkerNode {
public:
    MarkerNode();
    ~MarkerNode();
    MarkerNode(const MarkerNode &otherMarkerNode);
    
    unsigned char childrenVisitedMask;
    MarkerNode *children[8];
};

#endif /* defined(__hifi__MarkerNode__) */
