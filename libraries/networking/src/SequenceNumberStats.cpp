//
//  SequenceNumberStats.cpp
//  libraries/networking/src
//
//  Created by Yixin Wang on 6/25/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SequenceNumberStats.h"

#include <limits>

#include <LogHandler.h>

#include "NetworkLogging.h"

float PacketStreamStats::getLostRate() const {
    return (_expectedReceived == 0) ? 0.0f : (float)_lost / (float)_expectedReceived;
}

SequenceNumberStats::SequenceNumberStats(int statsHistoryLength, bool canDetectOutOfSync)
    : _lastReceivedSequence(0),
    _missingSet(),
    _stats(),
    _lastSenderUUID(),
    _statsHistory(statsHistoryLength),
    _lastUnreasonableSequence(0),
    _consecutiveUnreasonableOnTime(0)
{
}

void SequenceNumberStats::reset() {
    _missingSet.clear();
    _stats = PacketStreamStats();
    _lastSenderUUID = QUuid();
    _statsHistory.clear();
    _lastUnreasonableSequence = 0;
    _consecutiveUnreasonableOnTime = 0;
}

static const int UINT16_RANGE = std::numeric_limits<uint16_t>::max() + 1;

SequenceNumberStats::ArrivalInfo SequenceNumberStats::sequenceNumberReceived(quint16 incoming, QUuid senderUUID, const bool wantExtraDebugging) {

    SequenceNumberStats::ArrivalInfo arrivalInfo;

    // if the sender node has changed, reset all stats
    if (senderUUID != _lastSenderUUID) {
        if (_stats._received > 0) {
            qCDebug(networking) << "sequence number stats was reset due to new sender node";
            qCDebug(networking) << "previous:" << _lastSenderUUID << "current:" << senderUUID;
            reset();
        }
        _lastSenderUUID = senderUUID;
    }

    // determine our expected sequence number... handle rollover appropriately
    quint16 expected = _stats._received > 0 ? _lastReceivedSequence + (quint16)1 : incoming;

    _stats._received++;

    if (incoming == expected) { // on time
        arrivalInfo._status = OnTime;
        _lastReceivedSequence = incoming;
        _stats._expectedReceived++;

    } else { // out of order

        if (wantExtraDebugging) {
            qCDebug(networking) << "out of order... got:" << incoming << "expected:" << expected;
        }

        int incomingInt = (int)incoming;
        int expectedInt = (int)expected;

        // check if the gap between incoming and expected is reasonable, taking possible rollover into consideration
        int absGap = abs(incomingInt - expectedInt);
        if (absGap >= UINT16_RANGE - MAX_REASONABLE_SEQUENCE_GAP) {
            // rollover likely occurred between incoming and expected.
            // correct the larger of the two so that it's within [-UINT16_RANGE, -1] while the other remains within [0, UINT16_RANGE-1]
            if (incomingInt > expectedInt) {
                incomingInt -= UINT16_RANGE;
            } else {
                expectedInt -= UINT16_RANGE;
            }
        } else if (absGap > MAX_REASONABLE_SEQUENCE_GAP) {
            arrivalInfo._status = Unreasonable;

            static const QString UNREASONABLE_SEQUENCE_REGEX { "unreasonable sequence number: \\d+ previous: \\d+" };
            static QString repeatedMessage = LogHandler::getInstance().addRepeatedMessageRegex(UNREASONABLE_SEQUENCE_REGEX);

            qCDebug(networking) << "unreasonable sequence number:" << incoming << "previous:" << _lastReceivedSequence;

            _stats._unreasonable++;
            
            receivedUnreasonable(incoming);
            return arrivalInfo;
        }


        // now that rollover has been corrected for (if it occurred), incomingInt and expectedInt can be
        // compared to each other directly, though one of them might be negative

        arrivalInfo._seqDiffFromExpected = incomingInt - expectedInt;

        if (incomingInt > expectedInt) { // early
            arrivalInfo._status = Early;

            if (wantExtraDebugging) {
                qCDebug(networking) << "this packet is earlier than expected...";
                qCDebug(networking) << ">>>>>>>> missing gap=" << (incomingInt - expectedInt);
            }
            int skipped = incomingInt - expectedInt;
            _stats._early++;
            _stats._lost += skipped;
            _stats._expectedReceived += (skipped + 1);
            _lastReceivedSequence = incoming;

            // add all sequence numbers that were skipped to the missing sequence numbers list
            for (int missingInt = expectedInt; missingInt < incomingInt; missingInt++) {
                _missingSet.insert((quint16)(missingInt < 0 ? missingInt + UINT16_RANGE : missingInt));
            }

            // prune missing sequence list if it gets too big; sequence numbers that are older than MAX_REASONABLE_SEQUENCE_GAP
            // will be removed.
            if (_missingSet.size() > MAX_REASONABLE_SEQUENCE_GAP) {
                pruneMissingSet(wantExtraDebugging);
            }
        } else { // late
            if (wantExtraDebugging) {
                qCDebug(networking) << "this packet is later than expected...";
            }

            _stats._late++;

            // do not update _lastReceived; it shouldn't become smaller

            // remove this from missing sequence number if it's in there
            if (_missingSet.remove(incoming)) {
                arrivalInfo._status = Recovered;

                if (wantExtraDebugging) {
                    qCDebug(networking) << "found it in _missingSet";
                }
                _stats._lost--;
                _stats._recovered++;
            } else {
                // this late seq num is not in our missing set.  it is possibly a duplicate, or possibly a late
                // packet that should have arrived before our first received packet.  we'll count these
                // as unreasonable.

                arrivalInfo._status = Unreasonable;

                static const QString UNREASONABLE_SEQUENCE_REGEX { "unreasonable sequence number: \\d+ \\(possible duplicate\\)" };
                static QString repeatedMessage = LogHandler::getInstance().addRepeatedMessageRegex(UNREASONABLE_SEQUENCE_REGEX);

                qCDebug(networking) << "unreasonable sequence number:" << incoming << "(possible duplicate)";

                _stats._unreasonable++;

                receivedUnreasonable(incoming);
                return arrivalInfo;
            }
        }
    }

    // if we've made it here, we received a reasonable seq number.
    _consecutiveUnreasonableOnTime = 0;

    return arrivalInfo;
}

void SequenceNumberStats::receivedUnreasonable(quint16 incoming) {

    const int CONSECUTIVE_UNREASONABLE_ON_TIME_THRESHOLD = 8;

    quint16 expected = _consecutiveUnreasonableOnTime > 0 ? _lastUnreasonableSequence + (quint16)1 : incoming;
    if (incoming == expected) {
        _consecutiveUnreasonableOnTime++;
        _lastUnreasonableSequence = incoming;

        if (_consecutiveUnreasonableOnTime >= CONSECUTIVE_UNREASONABLE_ON_TIME_THRESHOLD) {
            // we've received many unreasonable seq numbers in a row, all in order. we're probably out of sync with
            // the seq num sender.  update our state to get back in sync with the sender.

            _lastReceivedSequence = incoming;
            _missingSet.clear();

            _stats._received = CONSECUTIVE_UNREASONABLE_ON_TIME_THRESHOLD;
            _stats._unreasonable = 0;
            _stats._early = 0;
            _stats._late = 0;
            _stats._lost = 0;
            _stats._recovered = 0;
            _stats._expectedReceived = CONSECUTIVE_UNREASONABLE_ON_TIME_THRESHOLD;

            _statsHistory.clear();
            _consecutiveUnreasonableOnTime = 0;

            qCDebug(networking) << "re-synced with sequence number sender";
        }
    } else {
        _consecutiveUnreasonableOnTime = 0;
    }
}

void SequenceNumberStats::pruneMissingSet(const bool wantExtraDebugging) {
    if (wantExtraDebugging) {
        qCDebug(networking) << "pruning _missingSet! size:" << _missingSet.size();
    }

    // some older sequence numbers may be from before a rollover point; this must be handled.
    // some sequence numbers in this list may be larger than _incomingLastSequence, indicating that they were received
    // before the most recent rollover.
    int cutoff = (int)_lastReceivedSequence - MAX_REASONABLE_SEQUENCE_GAP;
    if (cutoff >= 0) {
        quint16 nonRolloverCutoff = (quint16)cutoff;
        QSet<quint16>::iterator i = _missingSet.begin();
        while (i != _missingSet.end()) {
            quint16 missing = *i;
            if (wantExtraDebugging) {
                qCDebug(networking) << "checking item:" << missing << "is it in need of pruning?";
                qCDebug(networking) << "old age cutoff:" << nonRolloverCutoff;
            }

            if (missing > _lastReceivedSequence || missing < nonRolloverCutoff) {
                i = _missingSet.erase(i);
                if (wantExtraDebugging) {
                    qCDebug(networking) << "pruning really old missing sequence:" << missing;
                }
            } else {
                i++;
            }
        }
    } else {
        quint16 rolloverCutoff = (quint16)(cutoff + UINT16_RANGE);
        QSet<quint16>::iterator i = _missingSet.begin();
        while (i != _missingSet.end()) {
            quint16 missing = *i;
            if (wantExtraDebugging) {
                qCDebug(networking) << "checking item:" << missing << "is it in need of pruning?";
                qCDebug(networking) << "old age cutoff:" << rolloverCutoff;
            }

            if (missing > _lastReceivedSequence && missing < rolloverCutoff) {
                i = _missingSet.erase(i);
                if (wantExtraDebugging) {
                    qCDebug(networking) << "pruning really old missing sequence:" << missing;
                }
            } else {
                i++;
            }
        }
    }
}

PacketStreamStats SequenceNumberStats::getStatsForHistoryWindow() const {

    const PacketStreamStats* newestStats = _statsHistory.getNewestEntry();
    const PacketStreamStats* oldestStats = _statsHistory.get(_statsHistory.getNumEntries() - 1);

    // this catches cases where history is length 1 or 0 (both are NULL in case of 0)
    if (newestStats == oldestStats) {
        return PacketStreamStats();
    }

    // calculate difference between newest stats and oldest stats to get window stats
    return *newestStats - *oldestStats;
}

PacketStreamStats SequenceNumberStats::getStatsForLastHistoryInterval() const {

    const PacketStreamStats* newestStats = _statsHistory.getNewestEntry();
    const PacketStreamStats* secondNewestStats = _statsHistory.get(1);

    // this catches cases where history is length 1 or 0 (both are NULL in case of 0)
    if (newestStats == NULL || secondNewestStats == NULL) {
        return PacketStreamStats();
    }

    return *newestStats - *secondNewestStats;
}

