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

SequenceNumberStats::SequenceNumberStats(int statsHistoryLength)
    : _lastReceived(std::numeric_limits<quint16>::max()),
    _missingSet(),
    _stats(),
    _lastSenderUUID(),
    _statsHistory(statsHistoryLength)
{
}

void SequenceNumberStats::reset() {
    _missingSet.clear();
    _stats = PacketStreamStats();
    _statsHistory.clear();
}

static const int UINT16_RANGE = std::numeric_limits<uint16_t>::max() + 1;

SequenceNumberStats::ArrivalInfo SequenceNumberStats::sequenceNumberReceived(quint16 incoming, QUuid senderUUID, const bool wantExtraDebugging) {

    SequenceNumberStats::ArrivalInfo arrivalInfo;

    // if the sender node has changed, reset all stats
    if (senderUUID != _lastSenderUUID) {
        qDebug() << "sequence number stats was reset due to new sender node";
        qDebug() << "previous:" << _lastSenderUUID << "current:" << senderUUID;
        reset();
        _lastSenderUUID = senderUUID;
    }

    // determine our expected sequence number... handle rollover appropriately
    quint16 expected = _stats._numReceived > 0 ? _lastReceived + (quint16)1 : incoming;

    _stats._numReceived++;

    if (incoming == expected) { // on time
        arrivalInfo._status = OnTime;
        _lastReceived = incoming;
    } else { // out of order

        if (wantExtraDebugging) {
            qDebug() << "out of order... got:" << incoming << "expected:" << expected;
        }

        int incomingInt = (int)incoming;
        int expectedInt = (int)expected;

        // check if the gap between incoming and expected is reasonable, taking possible rollover into consideration
        int absGap = std::abs(incomingInt - expectedInt);
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

            // ignore packet if gap is unreasonable
            qDebug() << "ignoring unreasonable sequence number:" << incoming
                << "previous:" << _lastReceived;
            _stats._numUnreasonable++;

            return arrivalInfo;
        }

        // now that rollover has been corrected for (if it occurred), incomingInt and expectedInt can be
        // compared to each other directly, though one of them might be negative

        arrivalInfo._seqDiffFromExpected = incomingInt - expectedInt;

        if (incomingInt > expectedInt) { // early
            arrivalInfo._status = Early;

            if (wantExtraDebugging) {
                qDebug() << "this packet is earlier than expected...";
                qDebug() << ">>>>>>>> missing gap=" << (incomingInt - expectedInt);
            }
            
            _stats._numEarly++;
            _stats._numLost += (incomingInt - expectedInt);
            _lastReceived = incoming;

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
                qDebug() << "this packet is later than expected...";
            }

            _stats._numLate++;

            // do not update _lastReceived; it shouldn't become smaller

            // remove this from missing sequence number if it's in there
            if (_missingSet.remove(incoming)) {
                arrivalInfo._status = Late;

                if (wantExtraDebugging) {
                    qDebug() << "found it in _missingSet";
                }
                _stats._numLost--;
                _stats._numRecovered++;
            } else {
                arrivalInfo._status = Duplicate;

                if (wantExtraDebugging) {
                    qDebug() << "sequence:" << incoming << "was NOT found in _missingSet and is probably a duplicate";
                }
                _stats._numDuplicate++;
            }
        }
    }
    return arrivalInfo;
}

void SequenceNumberStats::pruneMissingSet(const bool wantExtraDebugging) {
    if (wantExtraDebugging) {
        qDebug() << "pruning _missingSet! size:" << _missingSet.size();
    }

    // some older sequence numbers may be from before a rollover point; this must be handled.
    // some sequence numbers in this list may be larger than _incomingLastSequence, indicating that they were received
    // before the most recent rollover.
    int cutoff = (int)_lastReceived - MAX_REASONABLE_SEQUENCE_GAP;
    if (cutoff >= 0) {
        quint16 nonRolloverCutoff = (quint16)cutoff;
        QSet<quint16>::iterator i = _missingSet.begin();
        while (i != _missingSet.end()) {
            quint16 missing = *i;
            if (wantExtraDebugging) {
                qDebug() << "checking item:" << missing << "is it in need of pruning?";
                qDebug() << "old age cutoff:" << nonRolloverCutoff;
            }

            if (missing > _lastReceived || missing < nonRolloverCutoff) {
                i = _missingSet.erase(i);
                if (wantExtraDebugging) {
                    qDebug() << "pruning really old missing sequence:" << missing;
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
                qDebug() << "checking item:" << missing << "is it in need of pruning?";
                qDebug() << "old age cutoff:" << rolloverCutoff;
            }

            if (missing > _lastReceived && missing < rolloverCutoff) {
                i = _missingSet.erase(i);
                if (wantExtraDebugging) {
                    qDebug() << "pruning really old missing sequence:" << missing;
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
    PacketStreamStats windowStats;
    windowStats._numReceived = newestStats->_numReceived - oldestStats->_numReceived;
    windowStats._numUnreasonable = newestStats->_numUnreasonable - oldestStats->_numUnreasonable;
    windowStats._numEarly = newestStats->_numEarly - oldestStats->_numEarly;
    windowStats._numLate = newestStats->_numLate - oldestStats->_numLate;
    windowStats._numLost = newestStats->_numLost - oldestStats->_numLost;
    windowStats._numRecovered = newestStats->_numRecovered - oldestStats->_numRecovered;
    windowStats._numDuplicate = newestStats->_numDuplicate - oldestStats->_numDuplicate;

    return windowStats;
}
