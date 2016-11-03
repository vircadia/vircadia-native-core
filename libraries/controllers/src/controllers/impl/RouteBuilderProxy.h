//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_Controllers_Impl_RouteBuilderProxy_h
#define hifi_Controllers_Impl_RouteBuilderProxy_h

#include <QtCore/QObject>

#include "Filter.h"
#include "Route.h"
#include "Mapping.h"

#include "../UserInputMapper.h"

class QJSValue;
class QScriptValue;
class QJsonValue;

namespace controller {

class ScriptingInterface;

// TODO migrate functionality to a RouteBuilder class and make the proxy defer to that 
// (for easier use in both C++ and JS)
class RouteBuilderProxy : public QObject {
        Q_OBJECT
    public:
        RouteBuilderProxy(UserInputMapper& parent, Mapping::Pointer mapping, Route::Pointer route)
            : _parent(parent), _mapping(mapping), _route(route) { }

        Q_INVOKABLE void toQml(const QJSValue& destination);
        Q_INVOKABLE QObject* whenQml(const QJSValue& expression);

        Q_INVOKABLE void to(const QScriptValue& destination);
        Q_INVOKABLE QObject* debug(bool enable = true);
        Q_INVOKABLE QObject* peek(bool enable = true);
        Q_INVOKABLE QObject* when(const QScriptValue& expression);
        Q_INVOKABLE QObject* clamp(float min, float max);
        Q_INVOKABLE QObject* hysteresis(float min, float max);
        Q_INVOKABLE QObject* pulse(float interval);
        Q_INVOKABLE QObject* scale(float multiplier);
        Q_INVOKABLE QObject* invert();
        Q_INVOKABLE QObject* deadZone(float min);
        Q_INVOKABLE QObject* constrainToInteger();
        Q_INVOKABLE QObject* constrainToPositiveInteger();

    private:
        void to(const Endpoint::Pointer& destination);
        void conditional(const Conditional::Pointer& conditional);
        void addFilter(Filter::Pointer filter);
        UserInputMapper& _parent;
        Mapping::Pointer _mapping;
        Route::Pointer _route;
    };

}
#endif
