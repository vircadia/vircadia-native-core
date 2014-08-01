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


const int DEFAULT_MAX_RECURSION = 5;


class PacketStreamStats {
public:
    PacketStreamStats()
        : _expectedReceived(0),
        _unreasonable(0),
        _early(0),
        _late(0),
        _lost(0),
        _recovered(0),
        _duplicate(0)
    {}

    float getOutOfOrderRate() const { return (float)(_early + _late) / _expectedReceived; }
    float getEaryRate() const { return (float)_early / _expectedReceived; }
    float getLateRate() const { return (float)_late / _expectedReceived; }
    float getLostRate() const { return (float)_lost / _expectedReceived; }

    quint32 _expectedReceived;
    quint32 _unreasonable;
    quint32 _early;
    quint32 _late;
    quint32 _lost;
    quint32 _recovered;
    quint32 _duplicate;
};

class SequenceNumberStats {
public:
    enum ArrivalStatus {
        OnTime,
        Unreasonable,
        Early,
        Late,   // recovered
        Duplicate
    };

    class ArrivalInfo {
    public:
        ArrivalStatus _status;
        int _seqDiffFromExpected;
    };


    SequenceNumberStats(int statsHistoryLength = 0, int maxRecursion = DEFAULT_MAX_RECURSION);
    SequenceNumberStats(const SequenceNumberStats& other);
    SequenceNumberStats& operator=(const SequenceNumberStats& rhs);
    ~SequenceNumberStats();

public:
    void reset();
    ArrivalInfo sequenceNumberReceived(quint16 incoming, QUuid senderUUID = QUuid(), const bool wantExtraDebugging = false);
    void pruneMissingSet(const bool wantExtraDebugging = false);
    void pushStatsToHistory() { _statsHistory.insert(_stats); }

    quint32 getReceived() const { return _received; }
    float getUnreasonableRate() const { return _stats._unreasonable / _received; }

    quint32 getExpectedReceived() const { return _stats._expectedReceived; }
    quint32 getUnreasonable() const { return _stats._unreasonable; }
    quint32 getOutOfOrder() const { return _stats._early + _stats._late; }
    quint32 getEarly() const { return _stats._early; }
    quint32 getLate() const { return _stats._late; }
    quint32 getLost() const { return _stats._lost; }
    quint32 getRecovered() const { return _stats._recovered; }
    quint32 getDuplicate() const { return _stats._duplicate; }

    const PacketStreamStats& getStats() const { return _stats; }
    PacketStreamStats getStatsForHistoryWindow() const;
    const QSet<quint16>& getMissingSet() const { return _missingSet; }

private:
    int _received;

    quint16 _lastReceivedSequence;
    QSet<quint16> _missingSet;

    PacketStreamStats _stats;

    QUuid _lastSenderUUID;

    RingBufferHistory<PacketStreamStats> _statsHistory;


    // to deal with the incoming seq nums going out of sync with this tracker, we'll create another instance
    // of this class when we encounter an unreasonable 
    SequenceNumberStats* _unreasonableTracker;
    int _maxRecursion;

    bool hasUnreasonableTracker() const { return _unreasonableTracker != NULL; }
};

#endif // hifi_SequenceNumberStats_h
