//
//  PIDController.h
//  libraries/shared/src
//
//  Given a measure of system performance (such as frame rate, where bigger denotes more system work),
//  compute a value that the system can take as input to control the amount of work done (such as an 1/LOD-distance,
//  where bigger tends to give a higher measured system performance value). The controller's job is to compute a
//  controlled value such that the measured value stays near the specified setpoint, even as system load changes.
//  See http://www.wetmachine.com/inventing-the-future/mostly-reliable-performance-of-software-processes-by-dynamic-control-of-quality-parameters/
//
//  Created by Howard Stearns 11/13/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PIDController_h
#define hifi_PIDController_h

#include <limits>
#include <QVector>

// Although our coding standard shuns abbreviations, the control systems literature uniformly uses p, i, d, and dt rather than
// proportionalTerm, integralTerm, derivativeTerm, and deltaTime. Here we will be consistent with the literature.
class PIDController {

public:
    // These are the main interfaces:
    void setMeasuredValueSetpoint(float newValue) { _measuredValueSetpoint = newValue; }
    float update(float measuredValue, float dt, bool resetAccumulator = false); // returns the new computedValue
    void setHistorySize(QString label = QString(""), int size = 0) { _history.reserve(size); _history.resize(0); _label = label; } // non-empty does logging

    bool getIsLogging() { return !_label.isEmpty(); }
    float getMeasuredValueSetpoint() const { return _measuredValueSetpoint; }
    // In normal operation (where we can easily reach setpoint), controlledValue is typcially pinned at max.
    // Defaults to [0, max float], but for 1/LODdistance, it might be, say, [0, 0.2 or 0.1]
    float getControlledValueLowLimit() const { return _controlledValueLowLimit; }
    float getControlledValueHighLimit() const { return _controlledValueHighLimit; }
    float getAntiWindupFactor() const { return _antiWindupFactor; } // default 10
    float getKP() const { return _kp; }  // proportional to error. See comment above class.
    float getKI() const { return _ki; }  // to time integral of error
    float getKD() const { return _kd; }  // to time derivative of error
    float getAccumulatedValueHighLimit() const { return getAntiWindupFactor() * getMeasuredValueSetpoint(); }
    float getAccumulatedValueLowLimit() const { return -getAntiWindupFactor() * getMeasuredValueSetpoint(); }

    // There are several values that rarely change and might be thought of as "constants", but which do change during tuning, debugging, or other
    // special-but-expected circumstances. Thus the instance vars are not const.
    void setControlledValueLowLimit(float newValue) { _controlledValueLowLimit = newValue; }
    void setControlledValueHighLimit(float newValue) { _controlledValueHighLimit = newValue; }
    void setAntiWindupFactor(float newValue) { _antiWindupFactor = newValue; }
    void setKP(float newValue) { _kp = newValue; }
    void setKI(float newValue) { _ki = newValue; }
    void setKD(float newValue) { _kd = newValue; }

    class Row { // one row of accumulated history, used only for logging (if at all)
    public:
        float measured;
        float dt;
        float error;
        float accumulated;
        float changed;
        float p;
        float i;
        float d;
        float computed;
    };
protected:
    void reportHistory();
    void updateHistory(float measured, float dt, float error, float accumulatedError, float changeInErro, float p, float i, float d, float computedValue);
    float _measuredValueSetpoint { 0.0f };
    float _controlledValueLowLimit { 0.0f };
    float _controlledValueHighLimit { std::numeric_limits<float>::max() };
    float _antiWindupFactor { 10.0f };
    float _kp { 0.0f };
    float _ki { 0.0f };
    float _kd { 0.0f };

    // Controller operating state
    float _lastError{ 0.0f };
    float _lastAccumulation{ 0.0f };

    // reporting
    QVector<Row> _history{};
    QString _label{ "" };

};

#endif // hifi_PIDController_h
