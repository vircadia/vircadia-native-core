//
//  Query.cpp
//  interface/src/gpu
//
//  Created by Niraj Venkat on 7/7/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Query.h"

#include "GPULogging.h"
#include "Batch.h"

using namespace gpu;

Query::Query(const Handler& returnHandler) :
    _returnHandler(returnHandler)
{
}

Query::~Query()
{
}

double Query::getElapsedTime() const {
    return ((double) _queryResult) * 0.000001;
}

void Query::triggerReturnHandler(uint64_t queryResult) {
    _queryResult = queryResult;
    if (_returnHandler) {
        _returnHandler(*this);
    }
}


Timer::Timer() {
    _timerQueries.resize(6, std::make_shared<gpu::Query>([&] (const Query& query) {
        auto elapsedTime = query.getElapsedTime();
        _movingAverage.addSample(elapsedTime);

        static int frameNum = 0;
        frameNum++;
        frameNum %= 10;
        if (frameNum == 0) {
            auto value = _movingAverage.average;
            qCDebug(gpulogging) << "AO on gpu = " << value;
        }
    }));

}

void Timer::begin(gpu::Batch& batch) {
    batch.beginQuery(_timerQueries[_currentTimerQueryIndex]);
}
void Timer::end(gpu::Batch& batch) {
    batch.endQuery(_timerQueries[_currentTimerQueryIndex]);
    _currentTimerQueryIndex = (_currentTimerQueryIndex + 1) % _timerQueries.size();

    auto returnQuery = _timerQueries[_currentTimerQueryIndex];
    batch.getQuery(returnQuery);
}

void Timer::onQueryReturned(const Query& query) {

}