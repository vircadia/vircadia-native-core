//
//  MovingPercentileTests.cpp
//  tests/shared/src
//
//  Created by Yixin Wang on 6/4/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MovingPercentileTests.h"

#include "SharedUtil.h"
#include "MovingPercentile.h"

#include <limits>
#include <qqueue.h>
#include <../QTestExtensions.h>

QTEST_MAIN(MovingPercentileTests)

// Defines the test values we use for n:
static const QVector<int> testValues { 1, 2, 3, 4, 5, 10, 100 };

void MovingPercentileTests::testRunningMin() {
    for (auto n : testValues)
        testRunningMinForN(n);
}

void MovingPercentileTests::testRunningMax() {
    for (auto n : testValues)
        testRunningMaxForN(n);
}

void MovingPercentileTests::testRunningMedian() {
    for (auto n : testValues)
        testRunningMedianForN(n);
}


float MovingPercentileTests::random() {
    return rand() / (float)RAND_MAX;
}

void MovingPercentileTests::testRunningMinForN (int n) {
    
    // Stores the last n samples
    QQueue<float> samples;
    
    MovingPercentile movingMin (n, 0.0f);
    
    for (int s = 0; s < 3 * n; ++s) {
        float sample = random();
        
        samples.push_back(sample);
        if (samples.size() > n)
            samples.pop_front();
        
        if (samples.size() == 0) {
            QFAIL_WITH_MESSAGE("\n\n\n\tWTF\n\tsamples.size() = " << samples.size() << ", n = " << n);
        }
        
        movingMin.updatePercentile(sample);
        
        // Calculate the minimum of the moving samples
        float expectedMin = std::numeric_limits<float>::max();
        
        int prevSize = samples.size();
        for (auto val : samples)
            expectedMin = std::min(val, expectedMin);
        QCOMPARE(samples.size(), prevSize);
        
        QCOMPARE(movingMin.getValueAtPercentile(), expectedMin);
    }
}

void MovingPercentileTests::testRunningMaxForN (int n) {
    
    // Stores the last n samples
    QQueue<float> samples;
    
    MovingPercentile movingMax (n, 1.0f);
    
    for (int s = 0; s < 10000; ++s) {
        float sample = random();
        
        samples.push_back(sample);
        if (samples.size() > n)
            samples.pop_front();
        
        if (samples.size() == 0) {
            QFAIL_WITH_MESSAGE("\n\n\n\tWTF\n\tsamples.size() = " << samples.size() << ", n = " << n);
        }
        
        movingMax.updatePercentile(sample);
        
        // Calculate the maximum of the moving samples
        float expectedMax = std::numeric_limits<float>::min();
        for (auto val : samples)
            expectedMax = std::max(val, expectedMax);
        
        QCOMPARE(movingMax.getValueAtPercentile(), expectedMax);
    }
}

void MovingPercentileTests::testRunningMedianForN (int n) {
    // Stores the last n samples
    QQueue<float> samples;
    
    MovingPercentile movingMedian (n, 0.5f);
    
    for (int s = 0; s < 10000; ++s) {
        float sample = random();
        
        samples.push_back(sample);
        if (samples.size() > n)
            samples.pop_front();
        
        if (samples.size() == 0) {
            QFAIL_WITH_MESSAGE("\n\n\n\tWTF\n\tsamples.size() = " << samples.size() << ", n = " << n);
        }
        
        movingMedian.updatePercentile(sample);
        auto median = movingMedian.getValueAtPercentile();
        
        // Check the number of samples that are > or < median
        int samplesGreaterThan = 0;
        int samplesLessThan    = 0;
        
        for (auto value : samples) {
            if (value < median)
                ++samplesGreaterThan;
            else if (value > median)
                ++samplesLessThan;
        }
        
        QCOMPARE_WITH_LAMBDA(samplesLessThan, n / 2, [=]() {
            return samplesLessThan <= n / 2;
        });
        QCOMPARE_WITH_LAMBDA(samplesGreaterThan, (n - 1) / 2, [=]() {
            return samplesGreaterThan <= n / 2;
        });
    }
}

