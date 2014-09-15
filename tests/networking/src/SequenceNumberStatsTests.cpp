//
//  AudioRingBufferTests.cpp
//  tests/networking/src
//
//  Created by Yixin Wang on 6/24/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cassert>
#include <limits>

#include <SharedUtil.h>

#include "SequenceNumberStatsTests.h"

void SequenceNumberStatsTests::runAllTests() {
    rolloverTest();
    earlyLateTest();
    duplicateTest();
    pruneTest();
    resyncTest();
}

const quint32 UINT16_RANGE = std::numeric_limits<quint16>::max() + 1;


void SequenceNumberStatsTests::rolloverTest() {

    SequenceNumberStats stats;

    // insert enough samples to cause 3 rollovers
    quint16 seq = 79;   // start on some random number

    for (int R = 0; R < 2; R++) {
        for (quint32 i = 0; i < 3 * UINT16_RANGE; i++) {
            stats.sequenceNumberReceived(seq);
            seq = seq + (quint16)1;

            assert(stats.getEarly() == 0);
            assert(stats.getLate() == 0);
            assert(stats.getLost() == 0);
            assert(stats.getReceived() == i + 1);
            assert(stats.getRecovered() == 0);
        }
        stats.reset();
    }
}

void SequenceNumberStatsTests::earlyLateTest() {

    SequenceNumberStats stats;
    quint16 seq = 65530;
    quint32 numSent = 0;

    quint32 numEarly = 0;
    quint32 numLate = 0;
    quint32 numLost = 0;
    quint32 numRecovered = 0;

    for (int R = 0; R < 2; R++) {
        for (int T = 0; T < 10000; T++) {

            // insert 7 consecutive
            for (int i = 0; i < 7; i++) {
                stats.sequenceNumberReceived(seq);
                seq = seq + (quint16)1;
                numSent++;

                assert(stats.getEarly() == numEarly);
                assert(stats.getLate() == numLate);
                assert(stats.getLost() == numLost);
                assert(stats.getReceived() == numSent);
                assert(stats.getRecovered() == numRecovered);
            }

            // skip 10
            quint16 skipped = seq;
            seq = seq + (quint16)10;

            // insert 36 consecutive
            numEarly++;
            numLost += 10;
            for (int i = 0; i < 36; i++) {
                stats.sequenceNumberReceived(seq);
                seq = seq + (quint16)1;
                numSent++;

                assert(stats.getEarly() == numEarly);
                assert(stats.getLate() == numLate);
                assert(stats.getLost() == numLost);
                assert(stats.getReceived() == numSent);
                assert(stats.getRecovered() == numRecovered);
            }

            // send ones we skipped
            for (int i = 0; i < 10; i++) {
                stats.sequenceNumberReceived(skipped);
                skipped = skipped + (quint16)1;
                numSent++;
                numLate++;
                numLost--;
                numRecovered++;

                assert(stats.getEarly() == numEarly);
                assert(stats.getLate() == numLate);
                assert(stats.getLost() == numLost);
                assert(stats.getReceived() == numSent);
                assert(stats.getRecovered() == numRecovered);
            }
        }
        stats.reset();
        numSent = 0;
        numEarly = 0;
        numLate = 0;
        numLost = 0;
        numRecovered = 0;
    }
}

void SequenceNumberStatsTests::duplicateTest() {

    SequenceNumberStats stats;
    quint16 seq = 12345;
    quint32 numSent = 0;

    quint32 numDuplicate = 0;
    quint32 numEarly = 0;
    quint32 numLate = 0;
    quint32 numLost = 0;

    for (int R = 0; R < 2; R++) {
        for (int T = 0; T < 5; T++) {

            quint16 duplicate = seq;

            // insert 7 consecutive
            for (int i = 0; i < 7; i++) {
                stats.sequenceNumberReceived(seq);
                seq = seq + (quint16)1;
                numSent++;

                assert(stats.getUnreasonable() == numDuplicate);
                assert(stats.getEarly() == numEarly);
                assert(stats.getLate() == numLate);
                assert(stats.getLost() == numLost);
                assert(stats.getReceived() == numSent);
                assert(stats.getRecovered() == 0);
            }

            // skip 10
            seq = seq + (quint16)10;


            quint16 duplicate2 = seq;

            numEarly++;
            numLost += 10;
            // insert 36 consecutive
            for (int i = 0; i < 36; i++) {
                stats.sequenceNumberReceived(seq);
                seq = seq + (quint16)1;
                numSent++;

                assert(stats.getUnreasonable() == numDuplicate);
                assert(stats.getEarly() == numEarly);
                assert(stats.getLate() == numLate);
                assert(stats.getLost() == numLost);
                assert(stats.getReceived() == numSent);
                assert(stats.getRecovered() == 0);
            }

            // send 5 duplicates from before skip
            for (int i = 0; i < 5; i++) {
                stats.sequenceNumberReceived(duplicate);
                duplicate = duplicate + (quint16)1;
                numSent++;
                numDuplicate++;
                numLate++;

                assert(stats.getUnreasonable() == numDuplicate);
                assert(stats.getEarly() == numEarly);
                assert(stats.getLate() == numLate);
                assert(stats.getLost() == numLost);
                assert(stats.getReceived() == numSent);
                assert(stats.getRecovered() == 0);
            }

            // send 5 duplicates from after skip
            for (int i = 0; i < 5; i++) {
                stats.sequenceNumberReceived(duplicate2);
                duplicate2 = duplicate2 + (quint16)1;
                numSent++;
                numDuplicate++;
                numLate++;

                assert(stats.getUnreasonable() == numDuplicate);
                assert(stats.getEarly() == numEarly);
                assert(stats.getLate() == numLate);
                assert(stats.getLost() == numLost);
                assert(stats.getReceived() == numSent);
                assert(stats.getRecovered() == 0);
            }
        }
        stats.reset();
        numSent = 0;
        numDuplicate = 0;
        numEarly = 0;
        numLate = 0;
        numLost = 0;
    }
}

void SequenceNumberStatsTests::pruneTest() {
    
    SequenceNumberStats stats;
    quint16 seq = 54321;
    quint32 numSent = 0;

    quint32 numEarly = 0;
    quint32 numLost = 0;

    for (int R = 0; R < 2; R++) {
        for (int T = 0; T < 1000; T++) {
            // insert 1 seq
            stats.sequenceNumberReceived(seq);
            seq = seq + (quint16)1;
            numSent++;

            // skip 1000 seq
            seq = seq + (quint16)1000;
            quint16 highestSkipped = seq - (quint16)1;

            // insert 1 seq
            stats.sequenceNumberReceived(seq);
            seq = seq + (quint16)1;
            numSent++;
            numEarly++;
            numLost += 1000;

            // skip 10 seq
            seq = seq + (quint16)10;
            quint16 highestSkipped2 = seq - (quint16)1;

            // insert 1 seq
            // insert 1 seq
            stats.sequenceNumberReceived(seq);
            seq = seq + (quint16)1;
            numSent++;
            numEarly++;
            numLost += 10;

            const QSet<quint16>& missingSet = stats.getMissingSet();
            assert(missingSet.size() <= 1000);
            if (missingSet.size() > 1000) {
                qDebug() << "FAIL: missingSet larger than 1000.";
            }

            for (int i = 0; i < 10; i++) {
                assert(missingSet.contains(highestSkipped2));
                highestSkipped2 = highestSkipped2 - (quint16)1;
            }

            for (int i = 0; i < 989; i++) {
                assert(missingSet.contains(highestSkipped));
                highestSkipped = highestSkipped - (quint16)1;
            }
            for (int i = 0; i < 11; i++) {
                assert(!missingSet.contains(highestSkipped));
                highestSkipped = highestSkipped - (quint16)1;
            }
        }
        stats.reset();
        numSent = 0;
        numEarly = 0;
        numLost = 0;
    }
}

void SequenceNumberStatsTests::resyncTest() {

    SequenceNumberStats stats(0);

    quint16 sequence;

    sequence = 89;
    stats.sequenceNumberReceived(sequence);

    assert(stats.getUnreasonable() == 0);

    sequence = 2990;
    for (int i = 0; i < 10; i++) {
        stats.sequenceNumberReceived(sequence);
        sequence += (quint16)1;
    }

    assert(stats.getUnreasonable() == 0);


    sequence = 0;
    for (int R = 0; R < 7; R++) {
        stats.sequenceNumberReceived(sequence);
        sequence += (quint16)1;
    }

    assert(stats.getUnreasonable() == 7);

    sequence = 6000;
    for (int R = 0; R < 7; R++) {
        stats.sequenceNumberReceived(sequence);
        sequence += (quint16)1;
    }

    assert(stats.getUnreasonable() == 14);

    sequence = 9000;
    for (int i = 0; i < 10; i++) {
        stats.sequenceNumberReceived(sequence);
        sequence += (quint16)1;
    }
    assert(stats.getUnreasonable() == 0);
}
