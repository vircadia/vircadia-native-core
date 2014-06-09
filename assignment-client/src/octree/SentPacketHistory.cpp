//
//  SentPacketHistory.cpp
//  assignement-client/src/octree
//
//  Created by Yixin Wang on 6/5/2014
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SentPacketHistory.h"

SentPacketHistory::SentPacketHistory(int size)
    : _sentPackets(size),
    _newestPacketAt(0),
    _numExistingPackets(0),
    _newestSequenceNumber(0)
{
}

void SentPacketHistory::packetSent(OCTREE_PACKET_SEQUENCE sequenceNumber, const QByteArray& packet) {
    _newestSequenceNumber = sequenceNumber;

    // increment _newestPacketAt cyclically, insert new packet there.
    // this will overwrite the oldest packet in the buffer
    _newestPacketAt = (_newestPacketAt == _sentPackets.size() - 1) ? 0 : _newestPacketAt + 1;
    _sentPackets[_newestPacketAt] = packet;
    if (_numExistingPackets < _sentPackets.size()) {
        _numExistingPackets++;
    }
}


const QByteArray* SentPacketHistory::getPacket(OCTREE_PACKET_SEQUENCE sequenceNumber) const {
    OCTREE_PACKET_SEQUENCE seqDiff = _newestSequenceNumber - sequenceNumber;
    if (!(seqDiff >= 0 && seqDiff < _numExistingPackets)) {
        return NULL;
    }
    int packetAt = _newestPacketAt - seqDiff;
    if (packetAt < 0) { packetAt += _sentPackets.size(); }
    return &_sentPackets.at(packetAt);
}
