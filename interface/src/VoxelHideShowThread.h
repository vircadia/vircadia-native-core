//
//  VoxelHideShowThread.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 12/1/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel persistence
//

#ifndef __interface__VoxelHideShowThread__
#define __interface__VoxelHideShowThread__

#include <GenericThread.h>
#include "VoxelSystem.h"

/// Generalized threaded processor for handling received inbound packets. 
class VoxelHideShowThread : public virtual GenericThread {
public:

    VoxelHideShowThread(VoxelSystem* theSystem);
    
protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();

private:
    VoxelSystem* _theSystem;
};

#endif // __interface__VoxelHideShowThread__
