//
//  SentPacketHistory.cpp
//  libraries/networking/src
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

void SentPacketHistory::packetSent(uint16_t sequenceNumber, const QByteArray& packet) {

    if (sequenceNumber != 0 && sequenceNumber != _newestSequenceNumber + 1) {
        printf("\t packet history received unexpected seq number! prev: %d  received: %d\n", _newestSequenceNumber, sequenceNumber);
    }


    _newestSequenceNumber = sequenceNumber;

    // increment _newestPacketAt cyclically, insert new packet there.
    // this will overwrite the oldest packet in the buffer
    _newestPacketAt = (_newestPacketAt == _sentPackets.size() - 1) ? 0 : _newestPacketAt + 1;
    _sentPackets[_newestPacketAt] = packet;
    if (_numExistingPackets < _sentPackets.size()) {
        _numExistingPackets++;
    }
}


const QByteArray* SentPacketHistory::getPacket(uint16_t sequenceNumber) const {

    const int UINT16_RANGE = UINT16_MAX + 1;

    // if sequenceNumber > _newestSequenceNumber, assume sequenceNumber is from before the most recent rollover
    // correct the diff so that it correctly represents how far back in the history sequenceNumber is
    int seqDiff = (int)_newestSequenceNumber - (int)sequenceNumber;
    if (seqDiff < 0) {
        seqDiff += UINT16_RANGE;
    }
    // if desired sequence number is too old to be found in the history, return null
    if (seqDiff >= _numExistingPackets) {
        return NULL;
    }
    int packetAt = _newestPacketAt - seqDiff;
    if (packetAt < 0) {
        packetAt += _sentPackets.size();
    }
    return &_sentPackets.at(packetAt);
}
