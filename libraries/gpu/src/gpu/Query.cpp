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

double Query::getGPUElapsedTime() const {
    return ((double)_queryResult) / 1000000.0;
}
double Query::getBatchElapsedTime() const {
    return ((double)_usecBatchElapsedTime) / 1000000.0;
}

void Query::triggerReturnHandler(uint64_t queryResult, uint64_t batchElapsedTime) {
    _queryResult = queryResult;
    _usecBatchElapsedTime = batchElapsedTime;

    if (_returnHandler) {
        _returnHandler(*this);
    }
}


RangeTimer::RangeTimer() {
    for (int i = 0; i < QUERY_QUEUE_SIZE; i++) {
        _timerQueries.push_back(std::make_shared<gpu::Query>([&, i] (const Query& query) {
            _tailIndex ++;

            _movingAverageGPU.addSample(query.getGPUElapsedTime());
            _movingAverageBatch.addSample(query.getBatchElapsedTime());
        }));
    }
}

void RangeTimer::begin(gpu::Batch& batch) {
    _headIndex++;
    batch.beginQuery(_timerQueries[rangeIndex(_headIndex)]);
}
void RangeTimer::end(gpu::Batch& batch) {
    if (_headIndex < 0) {
        return;
    }
    batch.endQuery(_timerQueries[rangeIndex(_headIndex)]);
    
    if (_tailIndex < 0) {
        _tailIndex = _headIndex;
    }
    
    // Pull the previous tail query hopping to see it return
    if (_tailIndex != _headIndex) {
        batch.getQuery(_timerQueries[rangeIndex(_tailIndex)]);
    }
}

double RangeTimer::getGPUAverage() const {
    return _movingAverageGPU.average;
}

double RangeTimer::getBatchAverage() const {
    return _movingAverageBatch.average;
}