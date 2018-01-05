//
//  OctreeServerConsts.h
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeServerConsts_h
#define hifi_OctreeServerConsts_h

#include <SharedUtil.h>
#include <NodeList.h> // for MAX_PACKET_SIZE
#include <JurisdictionSender.h>

const int MAX_FILENAME_LENGTH = 1024;

/// This is the frequency (hz) that we check the octree server for changes to determine if we need to
/// send new "scene" information to the viewers. This will directly effect how quickly edits are 
/// sent down do viewers. By setting it to 90hz we allow edits happening at 90hz to be sent down
/// to viewers at a rate more closely matching the edit rate. It would probably be better to allow
/// clients to ask the server to send at a rate consistent with their current vsynch since clients
/// can't render any faster than their vsynch even if the server sent them more acurate information
const int INTERVALS_PER_SECOND = 90;
const int OCTREE_SEND_INTERVAL_USECS = (1000 * 1000)/INTERVALS_PER_SECOND;

#endif // hifi_OctreeServerConsts_h
