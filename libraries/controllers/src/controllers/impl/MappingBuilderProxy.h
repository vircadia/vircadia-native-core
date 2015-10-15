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
class QJsonValue;

namespace controller {

class ScriptingInterface;

// TODO migrate functionality to a MappingBuilder class and make the proxy defer to that 
// (for easier use in both C++ and JS)
class MappingBuilderProxy : public QObject {
    Q_OBJECT
public:
    MappingBuilderProxy(ScriptingInterface& parent, Mapping::Pointer mapping)
        : _parent(parent), _mapping(mapping) { }

    Q_INVOKABLE QObject* from(const QJSValue& source);
    Q_INVOKABLE QObject* from(const QScriptValue& source);
    Q_INVOKABLE QObject* join(const QJSValue& source1, const QJSValue& source2);

    Q_INVOKABLE QObject* enable(bool enable = true);
    Q_INVOKABLE QObject* disable() { return enable(false); }


    // JSON route creation point
    Q_INVOKABLE QObject* from(const QJsonValue& json);


    void parse(const QJsonObject& json);
//  void serialize(QJsonObject& json);

protected:
    QObject* from(const Endpoint::Pointer& source);

    friend class RouteBuilderProxy;
    ScriptingInterface& _parent;
    Mapping::Pointer _mapping;


    void parseRoute(const QJsonValue& json);

};

}

#endif
