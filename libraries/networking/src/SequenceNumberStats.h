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
#include "RingBufferHistory.h"
#include <quuid.h>

const int MAX_REASONABLE_SEQUENCE_GAP = 1000;


class PacketStreamStats {
public:
    PacketStreamStats()
        : _received(0),
        _unreasonable(0),
        _early(0),
        _late(0),
        _lost(0),
        _recovered(0),
        _expectedReceived(0)
    {}

    PacketStreamStats operator-(const PacketStreamStats& rhs) const {
        PacketStreamStats diff;
        diff._received = _received - rhs._received;
        diff._unreasonable = _unreasonable - rhs._unreasonable;
        diff._early = _early - rhs._early;
        diff._late = _late - rhs._late;
        diff._lost = _lost - rhs._lost;
        diff._recovered = _recovered - rhs._recovered;
        diff._expectedReceived = _expectedReceived - rhs._expectedReceived;
        return diff;
    }

    float getLostRate() const;

    quint32 _received;
    quint32 _unreasonable;
    quint32 _early;
    quint32 _late;
    quint32 _lost;
    quint32 _recovered;
    quint32 _expectedReceived;
};

class SequenceNumberStats {
public:
    enum ArrivalStatus {
        OnTime,
        Unreasonable,
        Early,
        Recovered,
    };

    class ArrivalInfo {
    public:
        ArrivalStatus _status;
        int _seqDiffFromExpected;
    };


    SequenceNumberStats(int statsHistoryLength = 0, bool canDetectOutOfSync = true);

    void reset();
    ArrivalInfo sequenceNumberReceived(quint16 incoming, QUuid senderUUID = QUuid(), const bool wantExtraDebugging = false);
    void pruneMissingSet(const bool wantExtraDebugging = false);
    void pushStatsToHistory() { _statsHistory.insert(_stats); }

    quint32 getReceived() const { return _stats._received; }
    quint32 getExpectedReceived() const { return _stats._expectedReceived; }
    quint32 getUnreasonable() const { return _stats._unreasonable; }
    quint32 getOutOfOrder() const { return _stats._early + _stats._late; }
    quint32 getEarly() const { return _stats._early; }
    quint32 getLate() const { return _stats._late; }
    quint32 getLost() const { return _stats._lost; }
    quint32 getRecovered() const { return _stats._recovered; }

    const PacketStreamStats& getStats() const { return _stats; }
    PacketStreamStats getStatsForHistoryWindow() const;
    PacketStreamStats getStatsForLastHistoryInterval() const;
    const QSet<quint16>& getMissingSet() const { return _missingSet; }

private:
    void receivedUnreasonable(quint16 incoming);

private:
    quint16 _lastReceivedSequence;
    QSet<quint16> _missingSet;

    PacketStreamStats _stats;

    QUuid _lastSenderUUID;

    RingBufferHistory<PacketStreamStats> _statsHistory;

    quint16 _lastUnreasonableSequence;
    int _consecutiveUnreasonableOnTime;
};

#endif // hifi_SequenceNumberStats_h
