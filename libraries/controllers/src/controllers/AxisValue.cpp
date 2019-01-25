//
//  AxisValue.cpp
//
//  Created by David Rowe on 14 Dec 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AxisValue.h"

namespace controller {

    AxisValue::AxisValue(const float value, const quint64 timestamp) :
        value(value), timestamp(timestamp) { }

    bool AxisValue::operator==(const AxisValue& right) const {
        return value == right.value && timestamp == right.timestamp;
    }
}
