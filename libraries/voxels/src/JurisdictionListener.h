//
//  JurisdictionListener.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Voxel Packet Sender
//

#ifndef __shared__JurisdictionListener__
#define __shared__JurisdictionListener__

#include <PacketSender.h>
#include <ReceivedPacketProcessor.h>

#include "JurisdictionMap.h"

/// Sends out PACKET_TYPE_VOXEL_JURISDICTION_REQUEST packets to all voxel servers and then listens for and processes
/// the PACKET_TYPE_VOXEL_JURISDICTION packets it receives in order to maintain an accurate state of all jurisidictions
/// within the domain. As with other ReceivedPacketProcessor classes the user is responsible for reading inbound packets
/// and adding them to the processing queue by calling queueReceivedPacket()
class JurisdictionListener : public PacketSender, public ReceivedPacketProcessor {
public:
    static const int DEFAULT_PACKETS_PER_SECOND = 1;

    JurisdictionListener(PacketSenderNotify* notify = NULL);
    virtual bool process();

    const NodeToJurisdictionMap& getJurisdictions() const { return _jurisdictions; };

protected:
    /// Callback for processing of received packets. Will process any queued PACKET_TYPE_VOXEL_JURISDICTION and update the
    /// jurisdiction map member variable
    /// \param sockaddr& senderAddress the address of the sender
    /// \param packetData pointer to received data
    /// \param ssize_t packetLength size of received data
    /// \thread "this" individual processing thread
    virtual void processPacket(sockaddr& senderAddress, unsigned char*  packetData, ssize_t packetLength);

private:
    NodeToJurisdictionMap _jurisdictions;

    bool queueJurisdictionRequest();

};
#endif // __shared__JurisdictionListener__
