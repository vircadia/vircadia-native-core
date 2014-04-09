//
//  VoxelHideShowThread.h
//  interface/src/voxels
//
//  Created by Brad Hefta-Gaub on 12/1/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Threaded or non-threaded voxel persistence
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __interface__VoxelHideShowThread__
#define __interface__VoxelHideShowThread__

#include <GenericThread.h>
#include "VoxelSystem.h"

/// Generalized threaded processor for handling received inbound packets. 
class VoxelHideShowThread : public GenericThread {
    Q_OBJECT
public:

    VoxelHideShowThread(VoxelSystem* theSystem);
    
protected:
    /// Implements generic processing behavior for this thread.
    virtual bool process();

private:
    VoxelSystem* _theSystem;
};

#endif // __interface__VoxelHideShowThread__
