//
//  PIDController.cpp
//  libraries/shared/src
//
//  Created by Howard Stearns 11/13/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <glm/glm.hpp>
#include <QDebug>
#include "SharedLogging.h"
#include "PIDController.h"

float PIDController::update(float measuredValue, float dt, bool resetAccumulator, float FIXME1, float FIXME2) {
    const float error = getMeasuredValueSetpoint() - measuredValue;   // Sign is the direction we want measuredValue to go. Positive means go higher.

    const float p = getKP() * error; // term is Proportional to error

    const float accumulatedError = glm::clamp(error * dt + (resetAccumulator ? 0 : _lastAccumulation),  // integrate error
        getAccumulatedValueLowLimit(),   // but clamp by anti-windup limits
        getAccumulatedValueHighLimit());
    const float i = getKI() * accumulatedError; // term is Integral of error

    const float changeInError = (error - _lastError) / dt;  // positive value denotes increasing deficit
    const float d = getKD() * changeInError; // term is Derivative of Error

    const float computedValue = glm::clamp(p + i + d + getBias(),
        getControlledValueLowLimit(),
        getControlledValueHighLimit());

    if (_history.capacity()) {  // THIS SECTION ONLY FOR LOGGING
        // Don't report each update(), as the I/O messes with the results a lot.
        // Instead, add to history, and then dump out at once when full.
        // Typically, the first few values reported in each batch should be ignored.
        const int n = _history.size();
        _history.resize(n + 1);
        Row& next = _history[n];
        next.measured = measuredValue;
        next.FIXME1 = FIXME1;
        next.FIXME2 = FIXME2;
        next.dt = dt;
        next.error = error;
        next.accumulated = accumulatedError;
        next.changed = changeInError;
        next.p = p;
        next.i = i;
        next.d = d;
        next.computed = computedValue;
        if (_history.size() == _history.capacity()) { // report when buffer is full
            qCDebug(shared) << _label << "measured dt FIXME || error accumulated changed || p i d controlled";
            for (int i = 0; i < _history.size(); i++) {
                Row& row = _history[i];
                qCDebug(shared) << row.measured << row.dt << row.FIXME1 << row.FIXME2 <<
                    "||" << row.error << row.accumulated << row.changed <<
                    "||" << row.p << row.i << row.d << row.computed;
            }
            qCDebug(shared) << "Limits: accumulate" << getAccumulatedValueLowLimit() << getAccumulatedValueHighLimit() <<
                "controlled" << getControlledValueLowLimit() << getControlledValueHighLimit() <<
                "kp/ki/kd/bias" << getKP() << getKI() << getKD() << getBias();
            _history.resize(0);
        }
    }

    // update state for next time
    _lastError = error;
    _lastAccumulation = accumulatedError;
    return computedValue;
}