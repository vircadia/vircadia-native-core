//
//  MetavoxelSystem.h
//  interface
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelSystem__
#define __interface__MetavoxelSystem__

#include <MetavoxelData.h>

/// Renders a metavoxel tree.
class MetavoxelSystem {
public:

    void init();
    
private:
    
    MetavoxelData _data;    
};

#endif /* defined(__interface__MetavoxelSystem__) */
