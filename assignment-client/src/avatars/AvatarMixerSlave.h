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

class AvatarMixerSlave {
public:
    using ConstIter = NodeList::const_iterator;

    void configure(ConstIter begin, ConstIter end);

    void processIncomingPackets(const SharedNodePointer& node);
    void anotherJob(const SharedNodePointer& node);

    void harvestStats(int& nodesProcessed, int& packetsProcessed, quint64& processIncomingPacketsElapsedTime);

private:
    // frame state
    ConstIter _begin;
    ConstIter _end;

    int _nodesProcessed { 0 };
    int _packetsProcessed { 0 };
    quint64 _processIncomingPacketsElapsedTime { 0 };
};

#endif // hifi_AvatarMixerSlave_h
