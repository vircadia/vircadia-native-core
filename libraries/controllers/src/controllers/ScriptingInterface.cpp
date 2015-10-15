//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ScriptingInterface.h"

#include <mutex>
#include <set>

#include <QtCore/QRegularExpression>

#include <QJsonDocument>
#include <QJsonObject>

#include <GLMHelpers.h>
#include <DependencyManager.h>

#include "impl/MappingBuilderProxy.h"
#include "Logging.h"
#include "InputDevice.h"

namespace controller {

    class VirtualEndpoint : public Endpoint {
    public:
        VirtualEndpoint(const UserInputMapper::Input& id = UserInputMapper::Input(-1))
            : Endpoint(id) {
        }

        virtual float value() override { return _currentValue; }
        virtual void apply(float newValue, float oldValue, const Pointer& source) override { _currentValue = newValue; }

    private:
        float _currentValue{ 0.0f };
    };


    class JSEndpoint : public Endpoint {
    public:
        JSEndpoint(const QJSValue& callable) 
            : Endpoint(UserInputMapper::Input(-1)), _callable(callable) {}

        virtual float value() {
            float result = (float)_callable.call().toNumber();;
            return result;
        }

        virtual void apply(float newValue, float oldValue, const Pointer& source) {
            _callable.call(QJSValueList({ QJSValue(newValue) }));
        }

    private:
        QJSValue _callable;
    };

    class ScriptEndpoint : public Endpoint {
    public:
        ScriptEndpoint(const QScriptValue& callable)
            : Endpoint(UserInputMapper::Input(-1)), _callable(callable) {
        }

        virtual float value() {
            float result = (float)_callable.call().toNumber();
            return result;
        }

        virtual void apply(float newValue, float oldValue, const Pointer& source) {
            _callable.call(QScriptValue(), QScriptValueList({ QScriptValue(newValue) }));
        }

    private:
        QScriptValue _callable;
    };

    class CompositeEndpoint : public Endpoint, Endpoint::Pair {
    public:
        CompositeEndpoint(Endpoint::Pointer first, Endpoint::Pointer second)
            : Endpoint(UserInputMapper::Input(-1)), Pair(first, second) { }

        virtual float value() {
            float result = first->value() * -1.0 + second->value();
            return result;
        }

        virtual void apply(float newValue, float oldValue, const Pointer& source) {
            // Composites are read only
        }

    private:
        Endpoint::Pointer _first;
        Endpoint::Pointer _second;
    };


    QRegularExpression ScriptingInterface::SANITIZE_NAME_EXPRESSION{ "[\\(\\)\\.\\s]" };

    QVariantMap createDeviceMap(const UserInputMapper::DeviceProxy* device) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        QVariantMap deviceMap;
        for (const auto& inputMapping : device->getAvailabeInputs()) {
            const auto& input = inputMapping.first;
            const auto inputName = QString(inputMapping.second).remove(ScriptingInterface::SANITIZE_NAME_EXPRESSION);
            qCDebug(controllers) << "\tInput " << input.getChannel() << (int)input.getType()
                << QString::number(input.getID(), 16) << ": " << inputName;
            deviceMap.insert(inputName, input.getID());
        }
        return deviceMap;
    }

    ScriptingInterface::ScriptingInterface() {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        qCDebug(controllers) << "Setting up standard controller abstraction";
        auto standardDevice = userInputMapper->getStandardDevice();
        // Expose the IDs to JS
        _standard = createDeviceMap(standardDevice.get());
        // Create the endpoints
        for (const auto& inputMapping : standardDevice->getAvailabeInputs()) {
            const auto& standardInput = inputMapping.first;
            // Ignore aliases
            if (_endpoints.count(standardInput)) {
                continue;
            }
            _endpoints[standardInput] = std::make_shared<VirtualEndpoint>(standardInput);
        }

        // FIXME allow custom user actions?
        auto actionNames = userInputMapper->getActionNames();
        int actionNumber = 0;
        qCDebug(controllers) << "Setting up standard actions";
        for (const auto& actionName : actionNames) {
            UserInputMapper::Input actionInput(UserInputMapper::Input::ACTIONS_DEVICE, actionNumber++, UserInputMapper::ChannelType::AXIS);
            qCDebug(controllers) << "\tAction: " << actionName << " " << QString::number(actionInput.getID(), 16);
            // Expose the IDs to JS
            QString cleanActionName = QString(actionName).remove(ScriptingInterface::SANITIZE_NAME_EXPRESSION);
            _actions.insert(cleanActionName, actionInput.getID());

            // Create the endpoints
            // FIXME action endpoints need to accumulate values, and have them cleared at each frame
            _endpoints[actionInput] = std::make_shared<VirtualEndpoint>();
        }

        updateMaps();
    }

    QObject* ScriptingInterface::newMapping(const QString& mappingName) {
        if (_mappingsByName.count(mappingName)) {
            qCWarning(controllers) << "Refusing to recreate mapping named " << mappingName;
        }
        qDebug() << "Creating new Mapping " << mappingName;
        auto mapping = std::make_shared<Mapping>(mappingName); 
        _mappingsByName[mappingName] = mapping;
        return new MappingBuilderProxy(*this, mapping);
    }

    QObject* ScriptingInterface::parseMapping(const QString& json) {

        QJsonObject obj;
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        // check validity of the document
        if (!doc.isNull()) {
            if (doc.isObject()) {
                obj = doc.object();

                auto mapping = std::make_shared<Mapping>("default");
                auto mappingBuilder = new MappingBuilderProxy(*this, mapping);

                mappingBuilder->parse(obj);

                _mappingsByName[mapping->_name] = mapping;

            } else {
                qDebug() << "Mapping json Document is not an object" << endl;
            }
        } else {
            qDebug() << "Invalid JSON...\n" << json << endl;
        }

        return nullptr;
    }
    
    Q_INVOKABLE QObject* newMapping(const QJsonObject& json);

    void ScriptingInterface::enableMapping(const QString& mappingName, bool enable) {
        auto iterator = _mappingsByName.find(mappingName);
        if (_mappingsByName.end() == iterator) {
            qCWarning(controllers) << "Request to enable / disable unknown mapping " << mappingName;
            return;
        }

        auto mapping = iterator->second;
        if (enable) {
            _activeMappings.push_front(mapping);
        } else {
            auto activeIterator = std::find(_activeMappings.begin(), _activeMappings.end(), mapping);
            if (_activeMappings.end() == activeIterator) {
                qCWarning(controllers) << "Attempted to disable inactive mapping " << mappingName;
                return;
            }
            _activeMappings.erase(activeIterator);
        }
    }

    float ScriptingInterface::getValue(const int& source) const {
        // return (sin(secTimestampNow()) + 1.0f) / 2.0f;
        UserInputMapper::Input input(source);
        auto iterator = _endpoints.find(input);
        if (_endpoints.end() == iterator) {
            return 0.0;
        }

        const auto& endpoint = iterator->second;
        return getValue(endpoint);
    }

    float ScriptingInterface::getValue(const Endpoint::Pointer& endpoint) const {
        auto valuesIterator = _overrideValues.find(endpoint);
        if (_overrideValues.end() != valuesIterator) {
            return valuesIterator->second;
        }

        return endpoint->value();
    }

    float ScriptingInterface::getButtonValue(StandardButtonChannel source, uint16_t device) const {
        return getValue(UserInputMapper::Input(device, source, UserInputMapper::ChannelType::BUTTON).getID());
    }

    float ScriptingInterface::getAxisValue(StandardAxisChannel source, uint16_t device) const {
        return getValue(UserInputMapper::Input(device, source, UserInputMapper::ChannelType::AXIS).getID());
    }

    glm::mat4 ScriptingInterface::getPoseValue(StandardPoseChannel source, uint16_t device) const {
        return glm::mat4();
    }

    void ScriptingInterface::update() {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        static auto deviceNames = userInputMapper->getDeviceNames();

        if (deviceNames != userInputMapper->getDeviceNames()) {
            updateMaps();
            deviceNames = userInputMapper->getDeviceNames();
        }

        _overrideValues.clear();
        EndpointSet readEndpoints;
        EndpointSet writtenEndpoints;
        // Now process the current values for each level of the stack
        for (auto& mapping : _activeMappings) {
            for (const auto& mappingEntry : mapping->_channelMappings) {
                const auto& source = mappingEntry.first;

                // Endpoints can only be read once (though a given mapping can route them to 
                // multiple places).  Consider... If the default is to wire the A button to JUMP
                // and someone else wires it to CONTEXT_MENU, I don't want both to occur when 
                // I press the button.  The exception is if I'm wiring a control back to itself
                // in order to adjust my interface, like inverting the Y axis on an analog stick
                if (readEndpoints.count(source)) {
                    continue;
                }

                // Apply the value to all the routes
                const auto& routes = mappingEntry.second;

                for (const auto& route : routes) {
                    const auto& destination = route->_destination;

                    if (writtenEndpoints.count(destination)) {
                        continue;
                    }

                    // Standard controller destinations can only be can only be used once.
                    if (userInputMapper->getStandardDeviceID() == destination->getId().getDevice()) {
                        writtenEndpoints.insert(destination);
                    }

                    // Only consume the input if the route isn't a loopback.
                    // This allows mappings like `mapping.from(xbox.RY).invert().to(xbox.RY);`
                    bool loopback = source == destination;
                    if (!loopback) {
                        readEndpoints.insert(source);
                    }

                    // Fetch the value, may have been overriden by previous loopback routes
                    float value = getValue(source);

                    // Apply each of the filters.
                    const auto& filters = route->_filters;
                    for (const auto& filter : route->_filters) {
                        value = filter->apply(value);
                    }

                    if (loopback) {
                        _overrideValues[source] = value;
                    } else {
                        destination->apply(value, 0, source);
                    }
                }
            }
        }
    }

    Endpoint::Pointer ScriptingInterface::endpointFor(const QJSValue& endpoint) {
        if (endpoint.isNumber()) {
            return endpointFor(UserInputMapper::Input(endpoint.toInt()));
        }

        if (endpoint.isCallable()) {
            auto result = std::make_shared<JSEndpoint>(endpoint);
            return result;
        }

        qWarning() << "Unsupported input type " << endpoint.toString();
        return Endpoint::Pointer();
    }

    Endpoint::Pointer ScriptingInterface::endpointFor(const QScriptValue& endpoint) {
        if (endpoint.isNumber()) {
            return endpointFor(UserInputMapper::Input(endpoint.toInt32()));
        }

        if (endpoint.isFunction()) {
            auto result = std::make_shared<ScriptEndpoint>(endpoint);
            return result;
        }

        qWarning() << "Unsupported input type " << endpoint.toString();
        return Endpoint::Pointer();
    }

    UserInputMapper::Input ScriptingInterface::inputFor(const QString& inputName) {
        return DependencyManager::get<UserInputMapper>()->findDeviceInput(inputName);
    }

    Endpoint::Pointer ScriptingInterface::endpointFor(const UserInputMapper::Input& inputId) {
        auto iterator = _endpoints.find(inputId);
        if (_endpoints.end() == iterator) {
            qWarning() << "Unknown input: " << QString::number(inputId.getID(), 16);
            return Endpoint::Pointer();
        }
        return iterator->second;
    }

    Endpoint::Pointer ScriptingInterface::compositeEndpointFor(Endpoint::Pointer first, Endpoint::Pointer second) {
        EndpointPair pair(first, second);
        Endpoint::Pointer result;
        auto iterator = _compositeEndpoints.find(pair);
        if (_compositeEndpoints.end() == iterator) {
            result = std::make_shared<CompositeEndpoint>(first, second);
            _compositeEndpoints[pair] = result;
        } else {
            result = iterator->second;
        }
        return result;
    }

    bool ScriptingInterface::isPrimaryButtonPressed() const {
        return isButtonPressed(StandardButtonChannel::A);
    }
        
    glm::vec2 ScriptingInterface::getPrimaryJoystickPosition() const {
        return getJoystickPosition(0);
    }

    int ScriptingInterface::getNumberOfButtons() const {
        return StandardButtonChannel::NUM_STANDARD_BUTTONS;
    }

    bool ScriptingInterface::isButtonPressed(int buttonIndex) const {
        return getButtonValue((StandardButtonChannel)buttonIndex) == 0.0 ? false : true;
    }

    int ScriptingInterface::getNumberOfTriggers() const {
        return StandardCounts::TRIGGERS;
    }

    float ScriptingInterface::getTriggerValue(int triggerIndex) const {
        return getAxisValue(triggerIndex == 0 ? StandardAxisChannel::LT : StandardAxisChannel::RT);
    }

    int ScriptingInterface::getNumberOfJoysticks() const {
        return StandardCounts::ANALOG_STICKS;
    }

    glm::vec2 ScriptingInterface::getJoystickPosition(int joystickIndex) const {
        StandardAxisChannel xid = StandardAxisChannel::LX; 
        StandardAxisChannel yid = StandardAxisChannel::LY;
        if (joystickIndex != 0) {
            xid = StandardAxisChannel::RX;
            yid = StandardAxisChannel::RY;
        }
        vec2 result;
        result.x = getAxisValue(xid);
        result.y = getAxisValue(yid);
        return result;
    }

    int ScriptingInterface::getNumberOfSpatialControls() const {
        return StandardCounts::POSES;
    }

    glm::vec3 ScriptingInterface::getSpatialControlPosition(int controlIndex) const {
        // FIXME extract the position from the standard pose
        return vec3();
    }

    glm::vec3 ScriptingInterface::getSpatialControlVelocity(int controlIndex) const {
        // FIXME extract the velocity from the standard pose
        return vec3();
    }

    glm::vec3 ScriptingInterface::getSpatialControlNormal(int controlIndex) const {
        // FIXME extract the normal from the standard pose
        return vec3();
    }
    
    glm::quat ScriptingInterface::getSpatialControlRawRotation(int controlIndex) const {
        // FIXME extract the rotation from the standard pose
        return quat();
    }

    void ScriptingInterface::updateMaps() {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        auto devices = userInputMapper->getDevices();
        QSet<QString> foundDevices;
        for (const auto& deviceMapping : devices) {
            auto device = deviceMapping.second.get();
            auto deviceName = QString(device->getName()).remove(ScriptingInterface::SANITIZE_NAME_EXPRESSION);
            qCDebug(controllers) << "Device" << deviceMapping.first << ":" << deviceName;
            foundDevices.insert(device->getName());
            if (_hardware.contains(deviceName)) {
                continue;
            }

            // Expose the IDs to JS
            _hardware.insert(deviceName, createDeviceMap(device));

            // Create the endpoints
            for (const auto& inputMapping : device->getAvailabeInputs()) {
                const auto& input = inputMapping.first;
                // Ignore aliases
                if (_endpoints.count(input)) {
                    continue;
                }
                _endpoints[input] = std::make_shared<LambdaEndpoint>([=] {
                    auto deviceProxy = userInputMapper->getDeviceProxy(input);
                    if (!deviceProxy) {
                        return 0.0f;
                    }
                    return deviceProxy->getValue(input, 0);
                });
            }
        }
    }

} // namespace controllers

//var mapping = Controller.newMapping();
//mapping.map(hydra.LeftButton0, actions.ContextMenu);
//mapping.map(hydra.LeftButton0).to(xbox.RT);
//mapping.from(xbox.RT).constrainToBoolean().invert().to(actions.Foo)
//    mapping.from(xbox.RY).invert().deadZone(0.2).to(actions.Pitch)
//    mapping.from(xbox.RY).filter(function(newValue, oldValue) {
//    return newValue * 2.0
//}).to(actions.Pitch)

//mapping.from(function(time) {
//        return Math.cos(time);
//    }).to(actions.Pitch);

//    mapping.mapFromFunction(function() {
//        return x;
//    }, actions.ContextMenu);

//    mapping.from(xbox.LY).clamp(0, 1).to(actions.Forward);
//    mapping.from(xbox.LY).clamp(-1, 0).to(actions.Backward);
//    mapping.from(xbox.RY).clamp(0, 1).to(actions.Forward);
//    mapping.from(xbox.RS).to();
//    mapping.from(xbox.ALL).to();

//    mapping.from(xbox.RY).to(function(...) { ... });
//    mapping.from(xbox.RY).pass();

//    mapping.suppress() ≅ mapping.to(null)
//        mapping.pass() ≅ mapping.to(fromControl)

//        mapping.from(keyboard.RightParen).invert().to(actions.Yaw)
//        mapping.from(keyboard.LeftParen).to(actions.Yaw)

//        mapping.from(hydra.LX).pulse(MIN_SNAP_TIME, 3.0).to(Actions.Yaw)

//        mapping.from(keyboard.LeftParen).pulse(MIN_SNAP_TIME).to(Actions.Yaw)
//        // Enable and disable as above

//        mappingSnap.from(hydra.LX).to(function(newValue, oldValue) {
//        timeSinceLastYaw += deltaTime
