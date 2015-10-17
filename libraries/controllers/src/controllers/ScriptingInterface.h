//
//  AbstractControllerScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 12/17/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_AbstractControllerScriptingInterface_h
#define hifi_AbstractControllerScriptingInterface_h

#include <unordered_map>
#include <unordered_set>
#include <map>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QObject>
#include <QtCore/QVariant>

#include <QtQml/QJSValue>
#include <QtScript/QScriptValue>

#include "UserInputMapper.h"
#include "StandardControls.h"
#include "Mapping.h"

namespace controller {
    class InputController : public QObject {
        Q_OBJECT

    public:
        using Key = unsigned int;
        using Pointer = std::shared_ptr<InputController>;

        virtual void update() = 0;
        virtual Key getKey() const = 0;

    public slots:
        virtual bool isActive() const = 0;
        virtual glm::vec3 getAbsTranslation() const = 0;
        virtual glm::quat getAbsRotation() const = 0;
        virtual glm::vec3 getLocTranslation() const = 0;
        virtual glm::quat getLocRotation() const = 0;

    signals:
        //void spatialEvent(const SpatialEvent& event);
    };

    /// handles scripting of input controller commands from JS
    class ScriptingInterface : public QObject {
        Q_OBJECT
        Q_PROPERTY(QVariantMap Hardware READ getHardware CONSTANT FINAL)
        Q_PROPERTY(QVariantMap Actions READ getActions CONSTANT FINAL)
        Q_PROPERTY(QVariantMap Standard READ getStandard CONSTANT FINAL)

    public:
        ScriptingInterface();

        Q_INVOKABLE float getValue(const int& source) const;
        Q_INVOKABLE float getButtonValue(StandardButtonChannel source, uint16_t device = 0) const;
        Q_INVOKABLE float getAxisValue(StandardAxisChannel source, uint16_t device = 0) const;
        Q_INVOKABLE glm::mat4 getPoseValue(StandardPoseChannel source, uint16_t device = 0) const;
        Q_INVOKABLE QObject* newMapping(const QString& mappingName = QUuid::createUuid().toString());
        Q_INVOKABLE void enableMapping(const QString& mappingName, bool enable = true);
        Q_INVOKABLE void disableMapping(const QString& mappingName) {
            enableMapping(mappingName, false);
        }
        Q_INVOKABLE QObject* parseMapping(const QString& json);
        Q_INVOKABLE QObject* loadMapping(const QString& jsonUrl);


        Q_INVOKABLE bool isPrimaryButtonPressed() const;
        Q_INVOKABLE glm::vec2 getPrimaryJoystickPosition() const;

        Q_INVOKABLE int getNumberOfButtons() const;
        Q_INVOKABLE bool isButtonPressed(int buttonIndex) const;

        Q_INVOKABLE int getNumberOfTriggers() const;
        Q_INVOKABLE float getTriggerValue(int triggerIndex) const;

        Q_INVOKABLE int getNumberOfJoysticks() const;
        Q_INVOKABLE glm::vec2 getJoystickPosition(int joystickIndex) const;

        Q_INVOKABLE int getNumberOfSpatialControls() const;
        Q_INVOKABLE glm::vec3 getSpatialControlPosition(int controlIndex) const;
        Q_INVOKABLE glm::vec3 getSpatialControlVelocity(int controlIndex) const;
        Q_INVOKABLE glm::vec3 getSpatialControlNormal(int controlIndex) const;
        Q_INVOKABLE glm::quat getSpatialControlRawRotation(int controlIndex) const;

        Q_INVOKABLE const QVariantMap& getHardware() { return _hardware; }
        Q_INVOKABLE const QVariantMap& getActions() { return _actions; }
        Q_INVOKABLE const QVariantMap& getStandard() { return _standard; }

        static QRegularExpression SANITIZE_NAME_EXPRESSION;

    public slots:
        virtual void update();
        virtual void registerControllerTypes(QScriptEngine* engine) = 0;
        virtual void updateMaps();

    private:
        friend class MappingBuilderProxy;
        friend class RouteBuilderProxy;

        // FIXME move to unordered set / map
        using MappingMap = std::map<QString, Mapping::Pointer>;
        using MappingStack = std::list<Mapping::Pointer>;
        using InputToEndpointMap = std::map<UserInputMapper::Input, Endpoint::Pointer>;
        using EndpointSet = std::unordered_set<Endpoint::Pointer>;
        using ValueMap = std::map<Endpoint::Pointer, float>;
        using EndpointPair = std::pair<Endpoint::Pointer, Endpoint::Pointer>;
        using EndpointPairMap = std::map<EndpointPair, Endpoint::Pointer>;

        void update(Mapping::Pointer& mapping, EndpointSet& consumed);
        float getValue(const Endpoint::Pointer& endpoint) const;
        Endpoint::Pointer endpointFor(const QJSValue& endpoint);
        Endpoint::Pointer endpointFor(const QScriptValue& endpoint);
        Endpoint::Pointer endpointFor(const UserInputMapper::Input& endpoint);
        Endpoint::Pointer compositeEndpointFor(Endpoint::Pointer first, Endpoint::Pointer second);
 
        UserInputMapper::Input inputFor(const QString& inputName);

        QVariantMap _hardware;
        QVariantMap _actions;
        QVariantMap _standard;

        InputToEndpointMap _endpoints;
        EndpointPairMap _compositeEndpoints;

        ValueMap _overrideValues;
        MappingMap _mappingsByName;
        MappingStack _activeMappings;
    };
}


#endif // hifi_AbstractControllerScriptingInterface_h
