//
//  JurisdictionSender.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Voxel Packet Sender
//

#ifndef __shared__JurisdictionSender__
#define __shared__JurisdictionSender__

#include <PacketSender.h>
#include "JurisdictionMap.h"

/// Threaded processor for queueing and sending of outbound edit voxel packets. 
class JurisdictionSender : public PacketSender {
public:
    static const int DEFAULT_PACKETS_PER_SECOND = 1;

    JurisdictionSender(JurisdictionMap* map, PacketSenderNotify* notify = NULL);

    void setJurisdiction(JurisdictionMap* map) { _jurisdictionMap = map; }

    virtual bool process();

private:
    JurisdictionMap* _jurisdictionMap;
};
#endif // __shared__JurisdictionSender__
