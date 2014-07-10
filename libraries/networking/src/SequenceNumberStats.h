//
//  SequenceNumberStats.h
//  libraries/networking/src
//
//  Created by Yixin Wang on 6/25/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SequenceNumberStats_h
#define hifi_SequenceNumberStats_h

#include "SharedUtil.h"
#include <quuid.h>

const int MAX_REASONABLE_SEQUENCE_GAP = 1000;

class PacketStreamStats {
public:
    PacketStreamStats()
        : _numReceived(0),
        _numUnreasonable(0),
        _numEarly(0),
        _numLate(0),
        _numLost(0),
        _numRecovered(0),
        _numDuplicate(0)
    {}
    quint32 _numReceived;
    quint32 _numUnreasonable;
    quint32 _numEarly;
    quint32 _numLate;
    quint32 _numLost;
    quint32 _numRecovered;
    quint32 _numDuplicate;
};

class SequenceNumberStats {
public:
    SequenceNumberStats();

    void reset();
    void sequenceNumberReceived(quint16 incoming, QUuid senderUUID = QUuid(), const bool wantExtraDebugging = false);
    void pruneMissingSet(const bool wantExtraDebugging = false);

    quint32 getNumReceived() const { return _stats._numReceived; }
    quint32 getNumUnreasonable() const { return _stats._numUnreasonable; }
    quint32 getNumOutOfOrder() const { return _stats._numEarly + _stats._numLate; }
    quint32 getNumEarly() const { return _stats._numEarly; }
    quint32 getNumLate() const { return _stats._numLate; }
    quint32 getNumLost() const { return _stats._numLost; }
    quint32 getNumRecovered() const { return _stats._numRecovered; }
    quint32 getNumDuplicate() const { return _stats._numDuplicate; }
    const PacketStreamStats& getStats() const { return _stats; }
    const QSet<quint16>& getMissingSet() const { return _missingSet; }

private:
    quint16 _lastReceived;
    QSet<quint16> _missingSet;

    PacketStreamStats _stats;

    QUuid _lastSenderUUID;
};

#endif // hifi_SequenceNumberStats_h
