//
//  Created by Bradley Austin Davis on 2016/04/04
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#include <stdint.h>
#include <QtCore/QObject>

class FrameTimingsScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(float mean READ getMean CONSTANT)
    Q_PROPERTY(float max READ getMax CONSTANT)
    Q_PROPERTY(float min READ getMin CONSTANT)
    Q_PROPERTY(float standardDeviation READ getStandardDeviation CONSTANT)
public:
    Q_INVOKABLE void start();
    Q_INVOKABLE void addValue(uint64_t value);
    Q_INVOKABLE void finish();
    Q_INVOKABLE QVariantList getValues() const;


    uint64_t getMax() const { return _max; }
    uint64_t getMin() const { return _min; }
    float getStandardDeviation() const { return _stdDev; }
    float getMean() const { return _mean; }

protected:
    std::vector<uint64_t> _values;
    bool _active { false };
    uint64_t _max { 0 };
    uint64_t _min { 0 };
    float _stdDev { 0 };
    float _mean { 0 };
};
