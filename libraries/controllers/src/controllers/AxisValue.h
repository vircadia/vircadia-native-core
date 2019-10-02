//
//  AxisValue.h
//
//  Created by David Rowe on 13 Dec 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_controllers_AxisValue_h
#define hifi_controllers_AxisValue_h

#include <QtCore/qglobal.h>

namespace controller {

    struct AxisValue {
    public:
        float value { 0.0f };
        // The value can be timestamped to determine if consecutive identical values should be output (e.g., mouse movement).
        quint64 timestamp { 0 };
        bool valid { false };

        AxisValue() {}
        AxisValue(const float value, const quint64 timestamp, bool valid = true);

        bool operator ==(const AxisValue& right) const;
        bool operator !=(const AxisValue& right) const { return !(*this == right); }
    };
}

#endif // hifi_controllers_AxisValue_h
