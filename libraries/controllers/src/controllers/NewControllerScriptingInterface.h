//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Controllers_NewControllerScriptingInterface_h
#define hifi_Controllers_NewControllerScriptingInterface_h

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

#include <QtCore/QObject>
#include <QtCore/QVariant>

#include <QtQml/QJSValue>
#include <QtScript/QScriptValue>

#include <input-plugins/UserInputMapper.h>

#include "Mapping.h"

class QScriptValue;

namespace controller {
    class NewControllerScriptingInterface : public QObject {
        Q_OBJECT
        Q_PROPERTY(QVariantMap Hardware READ getHardware CONSTANT FINAL)
        Q_PROPERTY(QVariantMap Actions READ getActions CONSTANT FINAL)
        Q_PROPERTY(QVariantMap Standard READ getStandard CONSTANT FINAL)

    public:
        NewControllerScriptingInterface();
        Q_INVOKABLE float getValue(const int& source);

        Q_INVOKABLE void update();
        Q_INVOKABLE QObject* newMapping(const QString& mappingName);
        Q_INVOKABLE void enableMapping(const QString& mappingName, bool enable = true);
        Q_INVOKABLE void disableMapping(const QString& mappingName) {
            enableMapping(mappingName, false);
        }


        const QVariantMap& getHardware() { return _hardware; }
        const QVariantMap& getActions() { return _actions; }
        const QVariantMap& getStandard() { return _standard; }

    private:

        // FIXME move to unordered set / map
        using MappingMap = std::map<QString, Mapping::Pointer>;
        using MappingStack = std::list<Mapping::Pointer>;
        using InputToEndpointMap = std::map<UserInputMapper::Input, Endpoint::Pointer>;
        using EndpointSet = std::unordered_set<Endpoint::Pointer>;
        using ValueMap = std::map<Endpoint::Pointer, float>;
        using EndpointPair = std::pair<Endpoint::Pointer, Endpoint::Pointer>;
        using EndpointPairMap = std::map<EndpointPair, Endpoint::Pointer>;

        void update(Mapping::Pointer& mapping, EndpointSet& consumed);
        float getValue(const Endpoint::Pointer& endpoint);
        Endpoint::Pointer endpointFor(const QJSValue& endpoint);
        Endpoint::Pointer endpointFor(const QScriptValue& endpoint);
        Endpoint::Pointer endpointFor(const UserInputMapper::Input& endpoint);
        Endpoint::Pointer compositeEndpointFor(Endpoint::Pointer first, Endpoint::Pointer second);

        UserInputMapper::Input inputFor(const QString& inputName);

        friend class MappingBuilderProxy;
        friend class RouteBuilderProxy;
    private:
        uint16_t _nextFunctionId;
        InputToEndpointMap _endpoints;
        EndpointPairMap _compositeEndpoints;

        ValueMap _overrideValues;
        MappingMap _mappingsByName;
        MappingStack _activeMappings;

        QVariantMap _hardware;
        QVariantMap _actions;
        QVariantMap _standard;
    };
}


#endif
