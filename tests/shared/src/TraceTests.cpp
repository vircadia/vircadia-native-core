//
//  Created by Bradley Austin Davis on 2016/12/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TraceTests.h"

#include <QtTest/QtTest>
#include <QtGui/QDesktopServices>

#include <Profile.h>

#include <NumericalConstants.h>
#include <../QTestExtensions.h>

QTEST_MAIN(TraceTests)
Q_LOGGING_CATEGORY(trace_test, "trace.test")

const QString OUTPUT_FILE = "traces/testTrace.json.gz";

void TraceTests::testTraceSerialization() {
    auto tracer = DependencyManager::set<tracing::Tracer>();
    tracer->startTracing();
    {
        auto start = usecTimestampNow();
        PROFILE_RANGE(test, "TestEvent")
        for (size_t i = 0; i < 10000; ++i) {
            SAMPLE_PROFILE_COUNTER(0.1f, test, "TestCounter", { { "i", (int)i } })
        }
        auto duration = usecTimestampNow() - start;
        duration /= USECS_PER_MSEC;
        qDebug() << "Recording took " << duration << "ms";
    }
    tracer->stopTracing();
    {
        auto start = usecTimestampNow();
        tracer->serialize(OUTPUT_FILE);
        auto duration = usecTimestampNow() - start;
        duration /= USECS_PER_MSEC;
        qDebug() << "Serialization took " << duration << "ms";
    }
    qDebug() << "Done";
}

