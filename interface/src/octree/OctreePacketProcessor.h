//
//  OctreePacketProcessor.h
//  interface/src/voxels
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreePacketProcessor_h
#define hifi_OctreePacketProcessor_h

#include <ReceivedPacketProcessor.h>

/// Handles processing of incoming voxel packets for the interface application. As with other ReceivedPacketProcessor classes 
/// the user is responsible for reading inbound packets and adding them to the processing queue by calling queueReceivedPacket()
class OctreePacketProcessor : public ReceivedPacketProcessor {
    Q_OBJECT
protected:
    virtual void processPacket(const SharedNodePointer& sendingNode, const QByteArray& packet);
};
#endif // hifi_OctreePacketProcessor_h
