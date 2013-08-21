//  VoxelServer.h
//  voxel-server
//
//  Created by Brad Hefta-Gaub on 8/21/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __voxel_server__VoxelServer__
#define __voxel_server__VoxelServer__

#include <SharedUtil.h>
#include <NodeList.h> // for MAX_PACKET_SIZE
#include <EnvironmentData.h>
#include <JurisdictionSender.h>
#include <VoxelTree.h>

#include "VoxelServerPacketProcessor.h"


const int MAX_FILENAME_LENGTH = 1024;
const int VOXEL_LISTEN_PORT = 40106;
const int VOXEL_SIZE_BYTES = 3 + (3 * sizeof(float));
const int VOXELS_PER_PACKET = (MAX_PACKET_SIZE - 1) / VOXEL_SIZE_BYTES;
const int MIN_BRIGHTNESS = 64;
const float DEATH_STAR_RADIUS = 4.0;
const float MAX_CUBE = 0.05f;
const int VOXEL_SEND_INTERVAL_USECS = 17 * 1000; // approximately 60fps
const int SENDING_TIME_TO_SPARE = 5 * 1000; // usec of sending interval to spare for calculating voxels
const int INTERVALS_PER_SECOND = 1000 * 1000 / VOXEL_SEND_INTERVAL_USECS;
const int MAX_VOXEL_TREE_DEPTH_LEVELS = 4;
const int ENVIRONMENT_SEND_INTERVAL_USECS = 1000000;

extern const char* LOCAL_VOXELS_PERSIST_FILE;
extern const char* VOXELS_PERSIST_FILE;
extern char voxelPersistFilename[MAX_FILENAME_LENGTH];
extern int PACKETS_PER_CLIENT_PER_INTERVAL;

extern VoxelTree serverTree; // this IS a reaveraging tree 
extern bool wantVoxelPersist;
extern bool wantLocalDomain;
extern bool debugVoxelSending;
extern bool shouldShowAnimationDebug;
extern bool displayVoxelStats;
extern bool debugVoxelReceiving;
extern bool sendEnvironments;
extern bool sendMinimalEnvironment;
extern bool dumpVoxelsOnMove;
extern EnvironmentData environmentData[3];
extern int receivedPacketCount;
extern JurisdictionMap* jurisdiction;
extern JurisdictionSender* jurisdictionSender;
extern VoxelServerPacketProcessor* voxelServerPacketProcessor;
extern pthread_mutex_t treeLock;



#endif // __voxel_server__VoxelServer__
