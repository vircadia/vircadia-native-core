//  OctreeServerConsts.h
//  octree-server
//
//  Created by Brad Hefta-Gaub on 12/4/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __octree_server__OctreeServerConsts__
#define __octree_server__OctreeServerConsts__

#include <SharedUtil.h>
#include <NodeList.h> // for MAX_PACKET_SIZE
#include <JurisdictionSender.h>

const int MAX_FILENAME_LENGTH = 1024;
const int INTERVALS_PER_SECOND = 60;
const int OCTREE_SEND_INTERVAL_USECS = (1000 * 1000)/INTERVALS_PER_SECOND;
const int SENDING_TIME_TO_SPARE = 5 * 1000; // usec of sending interval to spare for calculating voxels

#endif // __octree_server__OctreeServerConsts__
