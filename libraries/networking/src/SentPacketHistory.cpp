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

#include <limits>

#include <QDebug>

#include "NetworkLogging.h"
#include "NLPacket.h"

SentPacketHistory::SentPacketHistory(int size)
    : _sentPackets(size),
    _newestSequenceNumber(std::numeric_limits<uint16_t>::max())
{

}

void SentPacketHistory::packetSent(uint16_t sequenceNumber, const NLPacket& packet) {

    // check if given seq number has the expected value.  if not, something's wrong with
    // the code calling this function
    uint16_t expectedSequenceNumber = _newestSequenceNumber + (uint16_t)1;
    if (sequenceNumber != expectedSequenceNumber) {
        qCDebug(networking) << "Unexpected sequence number passed to SentPacketHistory::packetSent()!"
            << "Expected:" << expectedSequenceNumber << "Actual:" << sequenceNumber;
    }
    _newestSequenceNumber = sequenceNumber;

    _sentPackets.insert(NLPacket::createCopy(packet));
}

const NLPacket* SentPacketHistory::getPacket(uint16_t sequenceNumber) const {

    const int UINT16_RANGE = std::numeric_limits<uint16_t>::max() + 1;

    // if sequenceNumber > _newestSequenceNumber, assume sequenceNumber is from before the most recent rollover
    // correct the diff so that it correctly represents how far back in the history sequenceNumber is
    int seqDiff = (int)_newestSequenceNumber - (int)sequenceNumber;
    if (seqDiff < 0) {
        seqDiff += UINT16_RANGE;
    }

    return _sentPackets.get(seqDiff)->get();
}
