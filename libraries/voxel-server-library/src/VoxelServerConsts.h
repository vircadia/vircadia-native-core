//  VoxelServerConsts.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __voxel_server__VoxelServerConsts__
#define __voxel_server__VoxelServerConsts__

#include <SharedUtil.h>
#include <NodeList.h> // for MAX_PACKET_SIZE
#include <JurisdictionSender.h>
#include <VoxelTree.h>

#include "VoxelServerPacketProcessor.h"


const int MAX_FILENAME_LENGTH = 1024;
const int INTERVALS_PER_SECOND = 60;
const int VOXEL_SEND_INTERVAL_USECS = (1000 * 1000)/INTERVALS_PER_SECOND;
const int SENDING_TIME_TO_SPARE = 5 * 1000; // usec of sending interval to spare for calculating voxels
const int ENVIRONMENT_SEND_INTERVAL_USECS = 1000000;

extern const char* LOCAL_VOXELS_PERSIST_FILE;
extern const char* VOXELS_PERSIST_FILE;

#endif // __voxel_server__VoxelServerConsts__
