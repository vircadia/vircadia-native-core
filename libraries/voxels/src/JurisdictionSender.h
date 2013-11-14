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

#include <queue>

#include <PacketSender.h>
#include <ReceivedPacketProcessor.h>
#include "JurisdictionMap.h"

/// Will process PACKET_TYPE_VOXEL_JURISDICTION_REQUEST packets and send out PACKET_TYPE_VOXEL_JURISDICTION packets
/// to requesting parties. As with other ReceivedPacketProcessor classes the user is responsible for reading inbound packets
/// and adding them to the processing queue by calling queueReceivedPacket()
class JurisdictionSender : public PacketSender, public ReceivedPacketProcessor {
public:
    static const int DEFAULT_PACKETS_PER_SECOND = 1;

    JurisdictionSender(JurisdictionMap* map, PacketSenderNotify* notify = NULL);
    ~JurisdictionSender();

    void setJurisdiction(JurisdictionMap* map) { _jurisdictionMap = map; }

    virtual bool process();

protected:
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);

    /// Locks all the resources of the thread.
    void lockRequestingNodes() { pthread_mutex_lock(&_requestingNodeMutex); }

    /// Unlocks all the resources of the thread.
    void unlockRequestingNodes() { pthread_mutex_unlock(&_requestingNodeMutex); }
    

private:
    pthread_mutex_t _requestingNodeMutex;
    JurisdictionMap* _jurisdictionMap;
    std::queue<QUuid> _nodesRequestingJurisdictions;
};
#endif // __shared__JurisdictionSender__
