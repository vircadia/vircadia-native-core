//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_UserInputMapper_h
#define hifi_UserInputMapper_h

#include <glm/glm.hpp>

#include <unordered_set>
#include <functional>
#include <memory>

#include <QtQml/QJSValue>
#include <QtScript/QScriptValue>

#include <DependencyManager.h>
#include <RegisteredMetaTypes.h>

#include "Pose.h"
#include "Input.h"
#include "InputDevice.h"
#include "DeviceProxy.h"
#include "StandardControls.h"
#include "Mapping.h"
#include "Endpoint.h"
#include "Actions.h"

namespace controller {

    class RouteBuilderProxy;
    class MappingBuilderProxy;

    class UserInputMapper : public QObject, public Dependency {
        Q_OBJECT
            SINGLETON_DEPENDENCY
            Q_ENUMS(Action)

    public:
        using InputPair = Input::NamedPair;
        // FIXME move to unordered set / map
        using EndpointToInputMap = std::map<Endpoint::Pointer, Input>;
        using MappingNameMap = std::map<QString, Mapping::Pointer>;
        using MappingDeviceMap = std::map<uint16_t, Mapping::Pointer>;
        using MappingStack = std::list<Mapping::Pointer>;
        using InputToEndpointMap = std::map<Input, Endpoint::Pointer>;
        using EndpointSet = std::unordered_set<Endpoint::Pointer>;
        using ValueMap = std::map<Endpoint::Pointer, float>;
        using EndpointPair = std::pair<Endpoint::Pointer, Endpoint::Pointer>;
        using EndpointPairMap = std::map<EndpointPair, Endpoint::Pointer>;
        using DevicesMap = std::map<int, DeviceProxy::Pointer>;
        using uint16 = uint16_t;
        using uint32 = uint32_t;

        static const uint16_t ACTIONS_DEVICE;
        static const uint16_t STANDARD_DEVICE;

        UserInputMapper();
        virtual ~UserInputMapper();


        static void registerControllerTypes(QScriptEngine* engine);


        void registerDevice(InputDevice* device);
        DeviceProxy::Pointer getDeviceProxy(const Input& input);
        QString getDeviceName(uint16 deviceID);

        Input::NamedVector getAvailableInputs(uint16 deviceID) const;
        Input::NamedVector getActionInputs() const { return getAvailableInputs(ACTIONS_DEVICE); }
        Input::NamedVector getStandardInputs() const { return getAvailableInputs(STANDARD_DEVICE); }

        int findDevice(QString name) const;
        QVector<QString> getDeviceNames();
        Input findDeviceInput(const QString& inputName) const;

        QVector<Action> getAllActions() const;
        QString getActionName(Action action) const;
        float getActionState(Action action) const { return _actionStates[toInt(action)]; }
        Pose getPoseState(Action action) const { return _poseStates[toInt(action)]; }
        int findAction(const QString& actionName) const;
        QVector<QString> getActionNames() const;

        void setActionState(Action action, float value) { _externalActionStates[toInt(action)] = value; }
        void deltaActionState(Action action, float delta) { _externalActionStates[toInt(action)] += delta; }
        void setActionState(Action action, const Pose& value) { _externalPoseStates[toInt(action)] = value; }

        static Input makeStandardInput(controller::StandardButtonChannel button);
        static Input makeStandardInput(controller::StandardAxisChannel axis);
        static Input makeStandardInput(controller::StandardPoseChannel pose);

        void removeDevice(int device);

        // Update means go grab all the device input channels and update the output channel values
        void update(float deltaTime);

        void setSensorToWorldMat(glm::mat4 sensorToWorldMat) { _sensorToWorldMat = sensorToWorldMat; }
        glm::mat4 getSensorToWorldMat() { return _sensorToWorldMat; }

        DevicesMap getDevices() { return _registeredDevices; }
        uint16 getStandardDeviceID() const { return STANDARD_DEVICE; }
        DeviceProxy::Pointer getStandardDevice() { return _registeredDevices[getStandardDeviceID()]; }

        Mapping::Pointer newMapping(const QString& mappingName);
        Mapping::Pointer parseMapping(const QString& json);
        Mapping::Pointer loadMapping(const QString& jsonFile);

        void enableMapping(const QString& mappingName, bool enable = true);
        float getValue(const Input& input) const;
        Pose getPose(const Input& input) const;

    signals:
        void actionEvent(int action, float state);
        void hardwareChanged();

    protected:
        virtual void update();
        // GetFreeDeviceID should be called before registering a device to use an ID not used by a different device.
        uint16 getFreeDeviceID() { return _nextFreeDeviceID++; }

        InputDevice::Pointer _standardController;
        DevicesMap _registeredDevices;
        uint16 _nextFreeDeviceID = STANDARD_DEVICE + 1;

        std::vector<float> _actionStates = std::vector<float>(toInt(Action::NUM_ACTIONS), 0.0f);
        std::vector<float> _externalActionStates = std::vector<float>(toInt(Action::NUM_ACTIONS), 0.0f);
        std::vector<float> _actionScales = std::vector<float>(toInt(Action::NUM_ACTIONS), 1.0f);
        std::vector<float> _lastActionStates = std::vector<float>(toInt(Action::NUM_ACTIONS), 0.0f);
        std::vector<Pose> _poseStates = std::vector<Pose>(toInt(Action::NUM_ACTIONS));
        std::vector<Pose> _externalPoseStates = std::vector<Pose>(toInt(Action::NUM_ACTIONS));

        glm::mat4 _sensorToWorldMat;

        int recordDeviceOfType(const QString& deviceName);
        QHash<const QString&, int> _deviceCounts;

        float getValue(const Endpoint::Pointer& endpoint) const;
        Pose getPose(const Endpoint::Pointer& endpoint) const;

        friend class RouteBuilderProxy;
        friend class MappingBuilderProxy;
        Endpoint::Pointer endpointFor(const QJSValue& endpoint);
        Endpoint::Pointer endpointFor(const QScriptValue& endpoint);
        Endpoint::Pointer endpointFor(const Input& endpoint) const;
        Endpoint::Pointer compositeEndpointFor(Endpoint::Pointer first, Endpoint::Pointer second);
        Mapping::Pointer parseMapping(const QJsonValue& json);
        Route::Pointer parseRoute(const QJsonValue& value);
        Conditional::Pointer parseConditional(const QJsonValue& value);
        Endpoint::Pointer parseEndpoint(const QJsonValue& value);

        InputToEndpointMap _endpointsByInput;
        EndpointToInputMap _inputsByEndpoint;
        EndpointPairMap _compositeEndpoints;

        ValueMap _overrideValues;
        MappingNameMap _mappingsByName;
        Mapping::Pointer _defaultMapping{ std::make_shared<Mapping>("Default") };
        MappingDeviceMap _mappingsByDevice;
        MappingStack _activeMappings;
    };

}

Q_DECLARE_METATYPE(controller::UserInputMapper::InputPair)
Q_DECLARE_METATYPE(controller::Pose)
Q_DECLARE_METATYPE(QVector<controller::UserInputMapper::InputPair>)
Q_DECLARE_METATYPE(controller::Input)
Q_DECLARE_METATYPE(controller::Action)
Q_DECLARE_METATYPE(QVector<controller::Action>)

// Cheating.
using UserInputMapper = controller::UserInputMapper;

#endif // hifi_UserInputMapper_h
