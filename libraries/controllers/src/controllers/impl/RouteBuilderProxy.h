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
#include "../Filter.h"
#include "../Route.h"

namespace Controllers {

class MappingBuilderProxy;

class RouteBuilderProxy : public QObject {
        Q_OBJECT
    public:
        RouteBuilderProxy(MappingBuilderProxy* parent, Route::Pointer route)
            : _parent(parent), _route(route) { }

        Q_INVOKABLE void to(const QString& destination);
        Q_INVOKABLE QObject* clamp(float min, float max);
        Q_INVOKABLE QObject* scale(float multiplier);
        Q_INVOKABLE QObject* invert();
        Q_INVOKABLE QObject* deadZone(float min);
        Q_INVOKABLE QObject* constrainToInteger();
        Q_INVOKABLE QObject* constrainToPositiveInteger();

    private:
        void addFilter(Filter::Lambda lambda);
        void addFilter(Filter::Pointer filter);
        Route::Pointer _route;
        MappingBuilderProxy* _parent;
    };

}
#endif
