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

#include <qqueue.h>

float MovingPercentileTests::random() {
    return rand() / (float)RAND_MAX;
}

void MovingPercentileTests::runAllTests() {

    QVector<int> valuesForN;
    
    valuesForN.append(1);
    valuesForN.append(2);
    valuesForN.append(3);
    valuesForN.append(4);
    valuesForN.append(5);
    valuesForN.append(10);
    valuesForN.append(100);


    QQueue<float> lastNSamples;

    for (int i=0; i<valuesForN.size(); i++) {

        int N = valuesForN.at(i);

        qDebug() << "testing moving percentile with N =" << N << "...";

        {
            bool fail = false;

            qDebug() << "\t testing running min...";

            lastNSamples.clear();
            MovingPercentile movingMin(N, 0.0f);

            for (int s = 0; s < 3*N; s++) {

                float sample = random();

                lastNSamples.push_back(sample);
                if (lastNSamples.size() > N) {
                    lastNSamples.pop_front();
                }

                movingMin.updatePercentile(sample);

                float experimentMin = movingMin.getValueAtPercentile();

                float actualMin = lastNSamples[0];
                for (int j = 0; j < lastNSamples.size(); j++) {
                    if (lastNSamples.at(j) < actualMin) {
                        actualMin = lastNSamples.at(j);
                    }
                }

                if (experimentMin != actualMin) {
                    qDebug() << "\t\t FAIL at sample" << s;
                    fail = true;
                    break;
                }
            }
            if (!fail) {
                qDebug() << "\t\t PASS";
            }
        }


        {
            bool fail = false;

            qDebug() << "\t testing running max...";

            lastNSamples.clear();
            MovingPercentile movingMax(N, 1.0f);

            for (int s = 0; s < 10000; s++) {

                float sample = random();

                lastNSamples.push_back(sample);
                if (lastNSamples.size() > N) {
                    lastNSamples.pop_front();
                }

                movingMax.updatePercentile(sample);

                float experimentMax = movingMax.getValueAtPercentile();

                float actualMax = lastNSamples[0];
                for (int j = 0; j < lastNSamples.size(); j++) {
                    if (lastNSamples.at(j) > actualMax) {
                        actualMax = lastNSamples.at(j);
                    }
                }

                if (experimentMax != actualMax) {
                    qDebug() << "\t\t FAIL at sample" << s;
                    fail = true;
                    break;
                }
            }
            if (!fail) {
                qDebug() << "\t\t PASS";
            }
        }


        {
            bool fail = false;

            qDebug() << "\t testing running median...";

            lastNSamples.clear();
            MovingPercentile movingMedian(N, 0.5f);

            for (int s = 0; s < 10000; s++) {

                float sample = random();

                lastNSamples.push_back(sample);
                if (lastNSamples.size() > N) {
                    lastNSamples.pop_front();
                }

                movingMedian.updatePercentile(sample);

                float experimentMedian = movingMedian.getValueAtPercentile();

                int samplesLessThan = 0;
                int samplesMoreThan = 0;

                for (int j=0; j<lastNSamples.size(); j++) {
                    if (lastNSamples.at(j) < experimentMedian) {
                        samplesLessThan++;
                    } else if (lastNSamples.at(j) > experimentMedian) {
                        samplesMoreThan++;
                    }
                }

                
                if (!(samplesLessThan <= N/2 && samplesMoreThan <= N-1/2)) {
                    qDebug() << "\t\t FAIL at sample" << s;
                    fail = true;
                    break;
                }
            }
            if (!fail) {
                qDebug() << "\t\t PASS";
            }
        }
    }
}

