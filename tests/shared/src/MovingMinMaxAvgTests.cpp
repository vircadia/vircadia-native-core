//
//  MovingMinMaxAvgTests.cpp
//  tests/shared/src
//
//  Created by Yixin Wang on 7/8/2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MovingMinMaxAvgTests.h"

#include <qqueue.h>

#include <NumericalConstants.h>

QTEST_MAIN(MovingMinMaxAvgTests)

quint64 MovingMinMaxAvgTests::randQuint64() {
    quint64 ret = 0;
    for (int i = 0; i < 32; i++) {
        ret = (ret + rand() % 4);
        ret *= 4;
    }
    return ret;
}

void MovingMinMaxAvgTests::testQuint64() {
    // quint64 test

    const int INTERVAL_LENGTH = 100;
    const int WINDOW_INTERVALS = 50;

    MovingMinMaxAvg<quint64> stats(INTERVAL_LENGTH, WINDOW_INTERVALS);

    quint64 min = std::numeric_limits<quint64>::max();
    quint64 max = 0;
    double average = 0.0;
    int totalSamples = 0;

    quint64 windowMin;
    quint64 windowMax;
    double windowAverage;

    QQueue<quint64> windowSamples;
    // fill window samples
    for (int i = 0; i < 100000; i++) {

        quint64 sample = randQuint64();

        windowSamples.enqueue(sample);
        if (windowSamples.size() > INTERVAL_LENGTH * WINDOW_INTERVALS) {
            windowSamples.dequeue();
        }

        stats.update(sample);

        min = std::min(min, sample);
        max = std::max(max, sample);
        average = (average * totalSamples + sample) / (totalSamples + 1);
        totalSamples++;

        QCOMPARE(stats.getMin(), min);
        QCOMPARE(stats.getMax(), max);
        
        QCOMPARE_WITH_ABS_ERROR((float) stats.getAverage() / (float) average, 1.0f, EPSILON);
        QCOMPARE_WITH_ABS_ERROR((float) stats.getAverage(), (float) average, EPSILON);
        
//        QCOMPARE(fabsf(
//                       (float)stats.getAverage() / (float)average - 1.0f
//                ) < EPSILON ||
//               fabsf(
//                     (float)stats.getAverage() - (float)average) < EPSILON);

        if ((i + 1) % INTERVAL_LENGTH == 0) {

            assert(stats.getNewStatsAvailableFlag());
            stats.clearNewStatsAvailableFlag();

            windowMin = std::numeric_limits<quint64>::max();
            windowMax = 0;
            windowAverage = 0.0;
            foreach(quint64 s, windowSamples) {
                windowMin = std::min(windowMin, s);
                windowMax = std::max(windowMax, s);
                windowAverage += (double)s;
            }
            windowAverage /= (double)windowSamples.size();

            assert(stats.getWindowMin() == windowMin);
            assert(stats.getWindowMax() == windowMax);
            assert(fabsf((float)stats.getAverage() / (float)average - 1.0f) < EPSILON ||
                   fabsf((float)stats.getAverage() - (float)average) < EPSILON);

        } else {
            assert(!stats.getNewStatsAvailableFlag());
        }
    }
}

void MovingMinMaxAvgTests::testInt() {
    // int test

    const int INTERVAL_LENGTH = 1;
    const int WINDOW_INTERVALS = 75;

    MovingMinMaxAvg<int> stats(INTERVAL_LENGTH, WINDOW_INTERVALS);

    int min = std::numeric_limits<int>::max();
    int max = 0;
    double average = 0.0;
    int totalSamples = 0;

    int windowMin;
    int windowMax;
    double windowAverage;

    QQueue<int> windowSamples;
    // fill window samples
    for (int i = 0; i < 100000; i++) {

        int sample = rand();

        windowSamples.enqueue(sample);
        if (windowSamples.size() > INTERVAL_LENGTH * WINDOW_INTERVALS) {
            windowSamples.dequeue();
        }

        stats.update(sample);

        min = std::min(min, sample);
        max = std::max(max, sample);
        average = (average * totalSamples + sample) / (totalSamples + 1);
        totalSamples++;

        assert(stats.getMin() == min);
        assert(stats.getMax() == max);
        assert(fabsf((float)stats.getAverage() / (float)average - 1.0f) < EPSILON);

        if ((i + 1) % INTERVAL_LENGTH == 0) {

            assert(stats.getNewStatsAvailableFlag());
            stats.clearNewStatsAvailableFlag();

            windowMin = std::numeric_limits<int>::max();
            windowMax = 0;
            windowAverage = 0.0;
            foreach(int s, windowSamples) {
                windowMin = std::min(windowMin, s);
                windowMax = std::max(windowMax, s);
                windowAverage += (double)s;
            }
            windowAverage /= (double)windowSamples.size();

            assert(stats.getWindowMin() == windowMin);
            assert(stats.getWindowMax() == windowMax);
            assert(fabsf((float)stats.getAverage() / (float)average - 1.0f) < EPSILON);

        } else {
            assert(!stats.getNewStatsAvailableFlag());
        }
    }
}

void MovingMinMaxAvgTests::testFloat() {
    // float test

    const int INTERVAL_LENGTH = 57;
    const int WINDOW_INTERVALS = 1;

    MovingMinMaxAvg<float> stats(INTERVAL_LENGTH, WINDOW_INTERVALS);

    float min = std::numeric_limits<float>::max();
    float max = 0;
    double average = 0.0;
    int totalSamples = 0;

    float windowMin;
    float windowMax;
    double windowAverage;

    QQueue<float> windowSamples;
    // fill window samples
    for (int i = 0; i < 100000; i++) {

        float sample = randFloat();

        windowSamples.enqueue(sample);
        if (windowSamples.size() > INTERVAL_LENGTH * WINDOW_INTERVALS) {
            windowSamples.dequeue();
        }

        stats.update(sample);

        min = std::min(min, sample);
        max = std::max(max, sample);
        average = (average * totalSamples + (double)sample) / (totalSamples + 1);
        totalSamples++;

        assert(stats.getMin() == min);
        assert(stats.getMax() == max);
        assert(fabsf((float)stats.getAverage() / (float)average - 1.0f) < EPSILON);

        if ((i + 1) % INTERVAL_LENGTH == 0) {

            assert(stats.getNewStatsAvailableFlag());
            stats.clearNewStatsAvailableFlag();

            windowMin = std::numeric_limits<float>::max();
            windowMax = 0;
            windowAverage = 0.0;
            foreach(float s, windowSamples) {
                windowMin = std::min(windowMin, s);
                windowMax = std::max(windowMax, s);
                windowAverage += (double)s;
            }
            windowAverage /= (double)windowSamples.size();

            assert(stats.getWindowMin() == windowMin);
            assert(stats.getWindowMax() == windowMax);
            assert(fabsf((float)stats.getAverage() / (float)average - 1.0f) < EPSILON);

        } else {
            assert(!stats.getNewStatsAvailableFlag());
        }
    }
}
