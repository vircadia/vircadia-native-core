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

#include "Mapping.h"
#include "Endpoint.h"

class QJSValue;
class QScriptValue;
class QJsonValue;

namespace controller {

class ScriptingInterface;
class UserInputMapper;

// TODO migrate functionality to a MappingBuilder class and make the proxy defer to that 
// (for easier use in both C++ and JS)
class MappingBuilderProxy : public QObject {
    Q_OBJECT
public:
    MappingBuilderProxy(UserInputMapper& parent, Mapping::Pointer mapping)
        : _parent(parent), _mapping(mapping) { }

    Q_INVOKABLE QObject* fromQml(const QJSValue& source);
    Q_INVOKABLE QObject* makeAxisQml(const QJSValue& source1, const QJSValue& source2);

    Q_INVOKABLE QObject* from(const QScriptValue& source);
    Q_INVOKABLE QObject* makeAxis(const QScriptValue& source1, const QScriptValue& source2);

    Q_INVOKABLE QObject* enable(bool enable = true);
    Q_INVOKABLE QObject* disable() { return enable(false); }

protected:
    QObject* from(const Endpoint::Pointer& source);

    friend class RouteBuilderProxy;
    UserInputMapper& _parent;
    Mapping::Pointer _mapping;


    void parseRoute(const QJsonValue& json);

};

}

#endif
