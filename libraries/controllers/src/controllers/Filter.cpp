//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Filter.h"

#include <QtCore/QObject>
#include <QtScript/QScriptValue>

namespace Controllers {

    class ScaleFilter : public Filter {
    public:
        virtual float apply(float newValue, float oldValue) {
            return newValue * _scale;
        }

    private:
        float _scale{ 1.0 };
    };

    class PulseFilter : public Filter {
    public:
        virtual float apply(float newValue, float oldValue) {
            // ???
        }

    private:

        float _lastEmitValue{ 0 };
        float _lastEmitTime{ 0 };
        float _interval{ -1.0f };
    };


    Filter::Pointer Filter::parse(const QJsonObject& json) {
        // FIXME parse the json object and determine the instance type to create
        return Filter::Pointer();
    }
}

