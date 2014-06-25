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

#include "SequenceNumberStatsTests.h"

#include "SharedUtil.h"
#include <limits>


void SequenceNumberStatsTests::runAllTests() {

    rolloverTest();
    earlyLateTest();
    duplicateTest();
    pruneTest();
}

const int UINT16_RANGE = std::numeric_limits<quint16>::max() + 1;


void SequenceNumberStatsTests::rolloverTest() {

    SequenceNumberStats stats;

    // insert enough samples to cause 3 rollovers
    quint16 seq = 79;   // start on some random number
    for (int i = 0; i < 3 * UINT16_RANGE; i++) {
        stats.sequenceNumberReceived(seq);
        seq = seq + (quint16)1;

        assert(stats.getNumDuplicate() == 0);
        assert(stats.getNumEarly() == 0);
        assert(stats.getNumLate() == 0);
        assert(stats.getNumLost() == 0);
        assert(stats.getNumReceived() == i+1);
        assert(stats.getNumRecovered() == 0);
    }
}

void SequenceNumberStatsTests::earlyLateTest() {

    SequenceNumberStats stats;
    quint16 seq = 65530;
    int numSent = 0;

    int numEarly = 0;
    int numLate = 0;
    int numLost = 0;
    int numRecovered = 0;


    for (int T = 0; T < 10000; T++) {

        // insert 7 consecutive
        for (int i = 0; i < 7; i++) {
            stats.sequenceNumberReceived(seq);
            seq = seq + (quint16)1;
            numSent++;

            assert(stats.getNumDuplicate() == 0);
            assert(stats.getNumEarly() == numEarly);
            assert(stats.getNumLate() == numLate);
            assert(stats.getNumLost() == numLost);
            assert(stats.getNumReceived() == numSent);
            assert(stats.getNumRecovered() == numRecovered);
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

            assert(stats.getNumDuplicate() == 0);
            assert(stats.getNumEarly() == numEarly);
            assert(stats.getNumLate() == numLate);
            assert(stats.getNumLost() == numLost);
            assert(stats.getNumReceived() == numSent);
            assert(stats.getNumRecovered() == numRecovered);
        }

        // send ones we skipped
        for (int i = 0; i < 10; i++) {
            stats.sequenceNumberReceived(skipped);
            skipped = skipped + (quint16)1;
            numSent++;
            numLate++;
            numLost--;
            numRecovered++;

            assert(stats.getNumDuplicate() == 0);
            assert(stats.getNumEarly() == numEarly);
            assert(stats.getNumLate() == numLate);
            assert(stats.getNumLost() == numLost);
            assert(stats.getNumReceived() == numSent);
            assert(stats.getNumRecovered() == numRecovered);
        }
    }
}

void SequenceNumberStatsTests::duplicateTest() {

    SequenceNumberStats stats;
    quint16 seq = 12345;
    int numSent = 0;

    int numDuplicate = 0;
    int numEarly = 0;
    int numLate = 0;
    int numLost = 0;


    for (int T = 0; T < 10000; T++) {

        quint16 duplicate = seq;

        // insert 7 consecutive
        for (int i = 0; i < 7; i++) {
            stats.sequenceNumberReceived(seq);
            seq = seq + (quint16)1;
            numSent++;

            assert(stats.getNumDuplicate() == numDuplicate);
            assert(stats.getNumEarly() == numEarly);
            assert(stats.getNumLate() == numLate);
            assert(stats.getNumLost() == numLost);
            assert(stats.getNumReceived() == numSent);
            assert(stats.getNumRecovered() == 0);
        }

        // skip 10
        quint16 skipped = seq;
        seq = seq + (quint16)10;


        quint16 duplicate2 = seq;

        numEarly++;
        numLost += 10;
        // insert 36 consecutive
        for (int i = 0; i < 36; i++) {
            stats.sequenceNumberReceived(seq);
            seq = seq + (quint16)1;
            numSent++;

            assert(stats.getNumDuplicate() == numDuplicate);
            assert(stats.getNumEarly() == numEarly);
            assert(stats.getNumLate() == numLate);
            assert(stats.getNumLost() == numLost);
            assert(stats.getNumReceived() == numSent);
            assert(stats.getNumRecovered() == 0);
        }

        // send 5 duplicates from before skip
        for (int i = 0; i < 5; i++) {
            stats.sequenceNumberReceived(duplicate);
            duplicate = duplicate + (quint16)1;
            numSent++;
            numDuplicate++;
            numLate++;

            assert(stats.getNumDuplicate() == numDuplicate);
            assert(stats.getNumEarly() == numEarly);
            assert(stats.getNumLate() == numLate);
            assert(stats.getNumLost() == numLost);
            assert(stats.getNumReceived() == numSent);
            assert(stats.getNumRecovered() == 0);
        }

        // send 5 duplicates from after skip
        for (int i = 0; i < 5; i++) {
            stats.sequenceNumberReceived(duplicate2);
            duplicate2 = duplicate2 + (quint16)1;
            numSent++;
            numDuplicate++;
            numLate++;

            assert(stats.getNumDuplicate() == numDuplicate);
            assert(stats.getNumEarly() == numEarly);
            assert(stats.getNumLate() == numLate);
            assert(stats.getNumLost() == numLost);
            assert(stats.getNumReceived() == numSent);
            assert(stats.getNumRecovered() == 0);
        }
    }
}

void SequenceNumberStatsTests::pruneTest() {
    
    SequenceNumberStats stats;
    quint16 seq = 54321;
    int numSent = 0;

    int numEarly = 0;
    int numLate = 0;
    int numLost = 0;

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
}