//
//  SentPacketHistory.h
//  assignement-client/src/octree
//
//  Created by Yixin Wang on 6/5/2014
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SentPacketHistory_h
#define hifi_SentPacketHistory_h

#include <qbytearray.h>
#include <qvector.h>

#include "OctreePacketData.h"

class SentPacketHistory {

public:
    SentPacketHistory(int size);

    void packetSent(OCTREE_PACKET_SEQUENCE sequenceNumber, const QByteArray& packet);
    const QByteArray* getPacket(OCTREE_PACKET_SEQUENCE sequenceNumber) const;

private:
    QVector<QByteArray> _sentPackets;    // circular buffer
    int _newestPacketAt;
    int _numExistingPackets;

    OCTREE_PACKET_SEQUENCE _newestSequenceNumber;
};

#endif
