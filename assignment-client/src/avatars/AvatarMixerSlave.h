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

class AvatarMixerClientData;

class AvatarMixerSlaveStats {
public:
    int nodesProcessed { 0 };
    int packetsProcessed { 0 };
    quint64 processIncomingPacketsElapsedTime { 0 };

    int nodesBroadcastedTo { 0 };
    int downstreamMixersBroadcastedTo { 0 };
    int numPacketsSent { 0 };
    int numBytesSent { 0 };
    int numIdentityPackets { 0 };
    int numOthersIncluded { 0 };
    int overBudgetAvatars { 0 };

    quint64 ignoreCalculationElapsedTime { 0 };
    quint64 avatarDataPackingElapsedTime { 0 };
    quint64 packetSendingElapsedTime { 0 };
    quint64 toByteArrayElapsedTime { 0 };
    quint64 jobElapsedTime { 0 };

    void reset() {
        // receiving job stats
        nodesProcessed = 0;
        packetsProcessed = 0;
        processIncomingPacketsElapsedTime = 0;

        // sending job stats
        nodesBroadcastedTo = 0;
        downstreamMixersBroadcastedTo = 0;
        numPacketsSent = 0;
        numBytesSent = 0;
        numIdentityPackets = 0;
        numOthersIncluded = 0;
        overBudgetAvatars = 0;

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

        nodesBroadcastedTo += rhs.nodesBroadcastedTo;
        downstreamMixersBroadcastedTo += rhs.downstreamMixersBroadcastedTo;
        numPacketsSent += rhs.numPacketsSent;
        numBytesSent += rhs.numBytesSent;
        numIdentityPackets += rhs.numIdentityPackets;
        numOthersIncluded += rhs.numOthersIncluded;
        overBudgetAvatars += rhs.overBudgetAvatars;

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
    void configureBroadcast(ConstIter begin, ConstIter end, 
                    p_high_resolution_clock::time_point lastFrameTimestamp, 
                    float maxKbpsPerNode, float throttlingRatio);

    void processIncomingPackets(const SharedNodePointer& node);
    void broadcastAvatarData(const SharedNodePointer& node);

    void harvestStats(AvatarMixerSlaveStats& stats);

private:
    int sendIdentityPacket(const AvatarMixerClientData* nodeData, const SharedNodePointer& destinationNode);
    int sendReplicatedIdentityPacket(const Node& agentNode, const AvatarMixerClientData* nodeData, const Node& destinationNode);

    void broadcastAvatarDataToAgent(const SharedNodePointer& node);
    void broadcastAvatarDataToDownstreamMixer(const SharedNodePointer& node);

    // frame state
    ConstIter _begin;
    ConstIter _end;

    p_high_resolution_clock::time_point _lastFrameTimestamp;
    float _maxKbpsPerNode { 0.0f };
    float _throttlingRatio { 0.0f };

    AvatarMixerSlaveStats _stats;
};

#endif // hifi_AvatarMixerSlave_h
