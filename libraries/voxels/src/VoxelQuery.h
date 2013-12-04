//
//  VoxelQuery.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelQuery__
#define __hifi__VoxelQuery__

#include <OctreeQuery.h>

class VoxelQuery : public OctreeQuery {

public:
    VoxelQuery(Node* owningNode = NULL);

    // currently just an alias
    
private:
    // privatize the copy constructor and assignment operator so they cannot be called
    VoxelQuery(const VoxelQuery&);
    VoxelQuery& operator= (const VoxelQuery&);
};

#endif /* defined(__hifi__VoxelQuery__) */
