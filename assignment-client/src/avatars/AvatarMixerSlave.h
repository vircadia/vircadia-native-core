//
//  AvatarMixerSlave.h
//  assignment-client/src/avatar
//
//  Created by Brad Hefta-Gaub on 2/14/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarMixerSlave_h
#define hifi_AvatarMixerSlave_h

/*
#include <AABox.h>
#include <AudioHRTF.h>
#include <AudioRingBuffer.h>
#include <ThreadedAssignment.h>
#include <UUIDHasher.h>
#include <NodeList.h>
*/

class AvatarMixerSlaveStats {
public:
    int nodesProcessed { 0 };
    int packetsProcessed { 0 };
    quint64 processIncomingPacketsElapsedTime { 0 };

    int numPacketsSent { 0 };
    quint64 ignoreCalculationElapsedTime { 0 };
    quint64 avatarDataPackingElapsedTime { 0 };
    quint64 packetSendingElapsedTime { 0 };
    quint64 toByteArrayElapsedTime { 0 };
    quint64 jobElapsedTime { 0 };

    void reset() {
        // receiving job stats
        nodesProcessed = 0;
        packetsProcessed = 0;
        numPacketsSent = 0;
        processIncomingPacketsElapsedTime = 0;

        // sending job stats
        numPacketsSent = 0;
        ignoreCalculationElapsedTime = 0;
        avatarDataPackingElapsedTime = 0;
        packetSendingElapsedTime = 0;
        toByteArrayElapsedTime = 0;
        jobElapsedTime = 0;
    }

    AvatarMixerSlaveStats& operator+=(const AvatarMixerSlaveStats& rhs) {
        nodesProcessed += rhs.nodesProcessed;
        packetsProcessed += rhs.packetsProcessed;
        processIncomingPacketsElapsedTime += rhs.processIncomingPacketsElapsedTime;

        numPacketsSent += rhs.numPacketsSent;
        ignoreCalculationElapsedTime += rhs.ignoreCalculationElapsedTime;
        avatarDataPackingElapsedTime += rhs.avatarDataPackingElapsedTime;
        packetSendingElapsedTime += rhs.packetSendingElapsedTime;
        toByteArrayElapsedTime += rhs.toByteArrayElapsedTime;
        jobElapsedTime += rhs.jobElapsedTime;
        return *this;
    }

};

class AvatarMixerSlave {
public:
    using ConstIter = NodeList::const_iterator;

    void configure(ConstIter begin, ConstIter end);

    void processIncomingPackets(const SharedNodePointer& node);
    void anotherJob(const SharedNodePointer& node);

    void harvestStats(AvatarMixerSlaveStats& stats);

private:
    // frame state
    ConstIter _begin;
    ConstIter _end;

    AvatarMixerSlaveStats _stats;
};

#endif // hifi_AvatarMixerSlave_h
