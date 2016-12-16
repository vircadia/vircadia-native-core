//
//  Created by Bradley Austin Davis on 2016/12/15
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TraceTests.h"

#include <QtTest/QtTest>
#include <Trace.h>

#include <SharedUtil.h>
#include <NumericalConstants.h>
#include <../QTestExtensions.h>

QTEST_MAIN(TraceTests)
Q_LOGGING_CATEGORY(tracertestlogging, "hifi.tracer.test")

void TraceTests::testTraceSerialization() {
    auto tracer = DependencyManager::set<tracing::Tracer>();
    tracer->startTracing();
    tracer->traceEvent(tracertestlogging(), "TestEvent", tracing::DurationBegin);
    {
        auto start = usecTimestampNow();
        for (size_t i = 0; i < 10000; ++i) {
            tracer->traceEvent(tracertestlogging(), "TestCounter", tracing::Counter, "", { { "i", i } });
        }
        auto duration = usecTimestampNow() - start;
        duration /= USECS_PER_MSEC;
        qDebug() << "Recording took " << duration << "ms";
    }
    tracer->traceEvent(tracertestlogging(), "TestEvent", tracing::DurationEnd);
    tracer->stopTracing();
    {
        auto start = usecTimestampNow();
        tracer->serialize("testTrace.json.gz");
        auto duration = usecTimestampNow() - start;
        duration /= USECS_PER_MSEC;
        qDebug() << "Serialization took " << duration << "ms";
    }
    qDebug() << "Done";
}

