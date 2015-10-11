//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_Controllers_Impl_MappingBuilderProxy_h
#define hifi_Controllers_Impl_MappingBuilderProxy_h

#include <QtCore/QObject>
#include <QtCore/QString>

#include "../Mapping.h"
#include "../Endpoint.h"

class QJSValue;
class QScriptValue;

namespace controller {

class NewControllerScriptingInterface;

class MappingBuilderProxy : public QObject {
    Q_OBJECT
public:
    MappingBuilderProxy(NewControllerScriptingInterface& parent, Mapping::Pointer mapping)
        : _parent(parent), _mapping(mapping) { }

    Q_INVOKABLE QObject* from(const QJSValue& source);
    Q_INVOKABLE QObject* from(const QScriptValue& source);

    Q_INVOKABLE QObject* join(const QJSValue& source1, const QJSValue& source2);
protected:
    QObject* from(const Endpoint::Pointer& source);

    friend class RouteBuilderProxy;
    NewControllerScriptingInterface& _parent;
    Mapping::Pointer _mapping;
};

}

#endif
