//  ParticleServerConsts.h
//  particle-server
//
//  Created by Brad Hefta-Gaub on 12/2/13
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __particle_server__ParticleServerConsts__
#define __particle_server__ParticleServerConsts__

#include <SharedUtil.h>
#include <NodeList.h> // for MAX_PACKET_SIZE
#include <JurisdictionSender.h>
#include <ParticleTree.h>

#include "ParticleServerPacketProcessor.h"

const int MAX_FILENAME_LENGTH = 1024;
const int INTERVALS_PER_SECOND = 60;
const int PARTICLE_SEND_INTERVAL_USECS = (1000 * 1000)/INTERVALS_PER_SECOND;
const int SENDING_TIME_TO_SPARE = 5 * 1000; // usec of sending interval to spare for calculating particles
const int ENVIRONMENT_SEND_INTERVAL_USECS = 1000000;

extern const char* LOCAL_PARTICLES_PERSIST_FILE;
extern const char* PARTICLES_PERSIST_FILE;

#endif // __particle_server__ParticleServerConsts__
