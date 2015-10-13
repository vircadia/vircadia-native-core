//
//  Created by Bradley Austin Davis 2015/10/09
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "NewControllerScriptingInterface.h"

#include <mutex>
#include <set>

#include <QtCore/QRegularExpression>

#include <GLMHelpers.h>
#include <DependencyManager.h>
#include <input-plugins/UserInputMapper.h>
#include <input-plugins/InputPlugin.h>
#include <input-plugins/KeyboardMouseDevice.h>
#include <plugins/PluginManager.h>

#include "impl/MappingBuilderProxy.h"
#include "Logging.h"

static const uint16_t ACTIONS_DEVICE = UserInputMapper::Input::INVALID_DEVICE - (uint16_t)1;

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

    QString sanatizeName(const QString& name) {
        QString cleanName{ name };
        cleanName.remove(QRegularExpression{ "[\\(\\)\\.\\s]" });
        return cleanName;
    }

    QVariantMap createDeviceMap(const UserInputMapper::DeviceProxy* device) {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        QVariantMap deviceMap;
        for (const auto& inputMapping : device->getAvailabeInputs()) {
            const auto& input = inputMapping.first;
            const auto inputName = sanatizeName(inputMapping.second);
            qCDebug(controllers) << "\tInput " << input.getChannel() << (int)input.getType()
                << QString::number(input.getID(), 16) << ": " << inputName;
            deviceMap.insert(inputName, input.getID());
        }
        return deviceMap;
    }

    NewControllerScriptingInterface::NewControllerScriptingInterface() {
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        auto devices = userInputMapper->getDevices();
        for (const auto& deviceMapping : devices) {
            auto device = deviceMapping.second.get();
            auto deviceName = sanatizeName(device->getName());
            qCDebug(controllers) << "Device" << deviceMapping.first << ":" << deviceName;
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

        auto actionNames = userInputMapper->getActionNames();
        int actionNumber = 0;
        qCDebug(controllers) << "Setting up standard actions";
        for (const auto& actionName : actionNames) {
            UserInputMapper::Input actionInput(ACTIONS_DEVICE, actionNumber++, UserInputMapper::ChannelType::AXIS);
            qCDebug(controllers) << "\tAction: " << actionName << " " << QString::number(actionInput.getID(), 16);
            // Expose the IDs to JS
            _actions.insert(sanatizeName(actionName), actionInput.getID());

            // Create the endpoints
            // FIXME action endpoints need to accumulate values, and have them cleared at each frame
            _endpoints[actionInput] = std::make_shared<VirtualEndpoint>();
        }
    }

    QObject* NewControllerScriptingInterface::newMapping(const QString& mappingName) {
        if (_mappingsByName.count(mappingName)) {
            qCWarning(controllers) << "Refusing to recreate mapping named " << mappingName;
        }
        qDebug() << "Creating new Mapping " << mappingName;
        Mapping::Pointer mapping = std::make_shared<Mapping>();
        _mappingsByName[mappingName] = mapping;
        return new MappingBuilderProxy(*this, mapping);
    }

    void NewControllerScriptingInterface::enableMapping(const QString& mappingName, bool enable) {
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

    float NewControllerScriptingInterface::getValue(const int& source) {
        // return (sin(secTimestampNow()) + 1.0f) / 2.0f;
        UserInputMapper::Input input(source);
        auto iterator = _endpoints.find(input);
        if (_endpoints.end() == iterator) {
            return 0.0;
        }

        const auto& endpoint = iterator->second;
        return getValue(endpoint);
    }

    float NewControllerScriptingInterface::getValue(const Endpoint::Pointer& endpoint) {
        auto valuesIterator = _overrideValues.find(endpoint);
        if (_overrideValues.end() != valuesIterator) {
            return valuesIterator->second;
        }

        return endpoint->value();
    }

        
    void NewControllerScriptingInterface::update() {
        static float last = secTimestampNow();
        float now = secTimestampNow();
        float delta = now - last;
        last = now;

        foreach(auto inputPlugin, PluginManager::getInstance()->getInputPlugins()) {
            inputPlugin->pluginUpdate(delta, false);
        }

        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        userInputMapper->update(delta);

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



    Endpoint::Pointer NewControllerScriptingInterface::endpointFor(const QJSValue& endpoint) {
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

    Endpoint::Pointer NewControllerScriptingInterface::endpointFor(const QScriptValue& endpoint) {
        if (endpoint.isNumber()) {
            return endpointFor(UserInputMapper::Input(endpoint.toInt32()));
        }

        qWarning() << "Unsupported input type " << endpoint.toString();
        return Endpoint::Pointer();
    }

    UserInputMapper::Input NewControllerScriptingInterface::inputFor(const QString& inputName) {
        return DependencyManager::get<UserInputMapper>()->findDeviceInput(inputName);
    }

    Endpoint::Pointer NewControllerScriptingInterface::endpointFor(const UserInputMapper::Input& inputId) {
        auto iterator = _endpoints.find(inputId);
        if (_endpoints.end() == iterator) {
            qWarning() << "Unknown input: " << QString::number(inputId.getID(), 16);
            return Endpoint::Pointer();
        }
        return iterator->second;
    }

    Endpoint::Pointer NewControllerScriptingInterface::compositeEndpointFor(Endpoint::Pointer first, Endpoint::Pointer second) {
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
