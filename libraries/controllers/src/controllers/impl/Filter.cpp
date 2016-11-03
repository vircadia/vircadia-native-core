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

#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <SharedUtil.h>

#include "filters/ClampFilter.h"
#include "filters/ConstrainToIntegerFilter.h"
#include "filters/ConstrainToPositiveIntegerFilter.h"
#include "filters/DeadZoneFilter.h"
#include "filters/HysteresisFilter.h"
#include "filters/InvertFilter.h"
#include "filters/PulseFilter.h"
#include "filters/ScaleFilter.h"

using namespace controller;

Filter::Factory Filter::_factory;

REGISTER_FILTER_CLASS_INSTANCE(ClampFilter, "clamp")
REGISTER_FILTER_CLASS_INSTANCE(ConstrainToIntegerFilter, "constrainToInteger")
REGISTER_FILTER_CLASS_INSTANCE(ConstrainToPositiveIntegerFilter, "constrainToPositiveInteger")
REGISTER_FILTER_CLASS_INSTANCE(DeadZoneFilter, "deadZone")
REGISTER_FILTER_CLASS_INSTANCE(HysteresisFilter, "hysteresis")
REGISTER_FILTER_CLASS_INSTANCE(InvertFilter, "invert")
REGISTER_FILTER_CLASS_INSTANCE(ScaleFilter, "scale")
REGISTER_FILTER_CLASS_INSTANCE(PulseFilter, "pulse")

const QString JSON_FILTER_TYPE = QStringLiteral("type");
const QString JSON_FILTER_PARAMS = QStringLiteral("params");


Filter::Pointer Filter::parse(const QJsonValue& json) {
    Filter::Pointer filter;
    if (json.isString()) {
        filter = Filter::getFactory().create(json.toString());
    } else if (json.isObject()) {
        QJsonObject jsonObj = json.toObject();
        // The filter is an object, now let s check for type and potential arguments
        auto filterType = jsonObj[JSON_FILTER_TYPE];
        filter = Filter::getFactory().create(filterType.toString());
        if (filter) {
            QJsonValue params = jsonObj;
            if (jsonObj.contains(JSON_FILTER_PARAMS)) {
                params = jsonObj[JSON_FILTER_PARAMS];
            }
            if (!filter->parseParameters(params)) {
                qWarning() << "Unable to parse filter parameters " << params;
                return Filter::Pointer();
            }
        }
    }
    return filter;
}

bool Filter::parseSingleFloatParameter(const QJsonValue& parameters, const QString& name, float& output) {
    if (parameters.isDouble()) {
        output = parameters.toDouble();
        return true;
    } else if (parameters.isArray()) {
        auto arrayParameters = parameters.toArray();
        if (arrayParameters.size() > 1) {
            output = arrayParameters[0].toDouble();
            return true;
        }
    } else if (parameters.isObject()) {
        static const QString JSON_MIN = QStringLiteral("interval");
        auto objectParameters = parameters.toObject();
        if (objectParameters.contains(name)) {
            output = objectParameters[name].toDouble();
            return true;
        }
    } 
    return false;
}



#if 0

namespace controller {

    class LambdaFilter : public Filter {
    public:
        //    LambdaFilter() {}12
        LambdaFilter(Lambda f) : _function(f) {};

        virtual float apply(float value) const {
            return _function(value);
        }

        virtual bool parseParameters(const QJsonArray& parameters) { return true; }

        //        REGISTER_FILTER_CLASS(LambdaFilter);
    private:
        Lambda _function;
    };

    class ScriptFilter : public Filter {
    public:

    };



    //class EasingFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override;

    //private:
    //    QEasingCurve _curve;
    //};

    //// GLSL style filters
    //class StepFilter : public Filter {
    //public:
    //    StepFilter(float edge) : _edge(edge) {};
    //    virtual float apply(float value) const override;

    //private:
    //    const float _edge;
    //};

    //class PowFilter : public Filter {
    //public:
    //    PowFilter(float exponent) : _exponent(exponent) {};
    //    virtual float apply(float value) const override;

    //private:
    //    const float _exponent;
    //};

    //class AbsFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override;
    //};

    //class SignFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override;
    //};

    //class FloorFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override {
    //        return floor(newValue);
    //    }
    //};

    //class CeilFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override {
    //        return ceil(newValue);
    //    }
    //};

    //class FractFilter : public Filter {
    //public:
    //    virtual float apply(float value) const override {
    //        return fract(newValue);
    //    }
    //};

    //class MinFilter : public Filter {
    //public:
    //    MinFilter(float mine) : _min(min) {};

    //    virtual float apply(float value) const override {
    //        return glm::min(_min, newValue);
    //    }

    //private:
    //    const float _min;
    //};

    //class MaxFilter : public Filter {
    //public:
    //    MaxFilter(float max) : _max(max) {};
    //    virtual float apply(float newValue, float oldValue) override;
    //private:
    //    const float _max;
    //};
}
#endif