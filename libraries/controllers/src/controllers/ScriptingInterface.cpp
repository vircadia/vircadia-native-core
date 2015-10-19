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
#include <QEventLoop>
#include <QThread>

#include <GLMHelpers.h>
#include <DependencyManager.h>

#include <ResourceManager.h>

#include "impl/MappingBuilderProxy.h"
#include "Logging.h"
#include "InputDevice.h"

namespace controller {

    class VirtualEndpoint : public Endpoint {
    public:
        VirtualEndpoint(const UserInputMapper::Input& id = UserInputMapper::Input::INVALID_INPUT)
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
            : Endpoint(UserInputMapper::Input::INVALID_INPUT), _callable(callable) {}

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

    float ScriptEndpoint::value() {
        updateValue();
        return _lastValue;
    }

    void ScriptEndpoint::updateValue() {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "updateValue", Qt::QueuedConnection);
            return;
        }

        _lastValue = (float)_callable.call().toNumber();
    }

    void ScriptEndpoint::apply(float newValue, float oldValue, const Pointer& source) {
        internalApply(newValue, oldValue, source->getInput().getID());
    }

    void ScriptEndpoint::internalApply(float newValue, float oldValue, int sourceID) {
        if (QThread::currentThread() != thread()) {
            QMetaObject::invokeMethod(this, "internalApply", Qt::QueuedConnection,
                Q_ARG(float, newValue),
                Q_ARG(float, oldValue),
                Q_ARG(int, sourceID));
            return;
        }
        _callable.call(QScriptValue(), 
                       QScriptValueList({ QScriptValue(newValue), QScriptValue(oldValue),  QScriptValue(sourceID) }));
    }

    class CompositeEndpoint : public Endpoint, Endpoint::Pair {
    public:
        CompositeEndpoint(Endpoint::Pointer first, Endpoint::Pointer second)
            : Endpoint(UserInputMapper::Input(UserInputMapper::Input::INVALID_INPUT)), Pair(first, second) { }

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

    class ActionEndpoint : public Endpoint {
    public:
        ActionEndpoint(const UserInputMapper::Input& id = UserInputMapper::Input::INVALID_INPUT)
            : Endpoint(id) {
        }

        virtual float value() override { return _currentValue; }
        virtual void apply(float newValue, float oldValue, const Pointer& source) override { 
            
            _currentValue += newValue;
            if (!(_input == UserInputMapper::Input::INVALID_INPUT)) {
                auto userInputMapper = DependencyManager::get<UserInputMapper>();
                userInputMapper->deltaActionState(UserInputMapper::Action(_input.getChannel()), newValue);
            }
        }

    private:
        float _currentValue{ 0.0f };
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
    

    ScriptingInterface::~ScriptingInterface() {
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
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
        // check validity of the document
        if (!doc.isNull()) {
            if (doc.isObject()) {
                obj = doc.object();

                auto mapping = std::make_shared<Mapping>("default");
                auto mappingBuilder = new MappingBuilderProxy(*this, mapping);

                mappingBuilder->parse(obj);

                _mappingsByName[mapping->_name] = mapping;

                return mappingBuilder;
            } else {
                qDebug() << "Mapping json Document is not an object" << endl;
            }
        } else {
            qDebug() << "Invalid JSON...\n";
            qDebug() << error.errorString();
            qDebug() << "JSON was:\n" << json << endl;

        }

        return nullptr;
    }

    QObject* ScriptingInterface::loadMapping(const QString& jsonUrl) {
        QObject* result = nullptr;
        auto request = ResourceManager::createResourceRequest(nullptr, QUrl(jsonUrl));
        if (request) {
            QEventLoop eventLoop;
            request->setCacheEnabled(false);
            connect(request, &ResourceRequest::finished, &eventLoop, &QEventLoop::quit);
            request->send();
            if (request->getState() != ResourceRequest::Finished) {
                eventLoop.exec();
            }

            if (request->getResult() == ResourceRequest::Success) {
                result = parseMapping(QString(request->getData()));
            } else {
                qCWarning(controllers) << "Failed to load mapping url <" << jsonUrl << ">" << endl;
            }
            request->deleteLater();
        }
        return result;
    }

    Q_INVOKABLE QObject* newMapping(const QJsonObject& json);

    void ScriptingInterface::enableMapping(const QString& mappingName, bool enable) {
        qCDebug(controllers) << "Attempting to enable mapping " << mappingName;
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

    Pose ScriptingInterface::getPoseValue(StandardPoseChannel source, uint16_t device) const {
        return Pose();
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
                    // THis could happen if the route destination failed to create
                    // FIXME: Maybe do not create the route if the destination failed and avoid this case ?
                    if (!destination) {
                        continue;
                    }

                    if (writtenEndpoints.count(destination)) {
                        continue;
                    }

                    // Standard controller destinations can only be can only be used once.
                    if (userInputMapper->getStandardDeviceID() == destination->getInput().getDevice()) {
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
            auto deviceID = deviceMapping.first;
            if (deviceID != userInputMapper->getStandardDeviceID()) {
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
    }

    QVector<UserInputMapper::Action> ScriptingInterface::getAllActions() {
        return DependencyManager::get<UserInputMapper>()->getAllActions();
    }

    QString ScriptingInterface::getDeviceName(unsigned int device) {
        return DependencyManager::get<UserInputMapper>()->getDeviceName((unsigned short)device);
    }

    QVector<UserInputMapper::InputPair> ScriptingInterface::getAvailableInputs(unsigned int device) {
        return DependencyManager::get<UserInputMapper>()->getAvailableInputs((unsigned short)device);
    }

    int ScriptingInterface::findDevice(QString name) {
        return DependencyManager::get<UserInputMapper>()->findDevice(name);
    }

    QVector<QString> ScriptingInterface::getDeviceNames() {
        return DependencyManager::get<UserInputMapper>()->getDeviceNames();
    }

    float ScriptingInterface::getActionValue(int action) {
        return DependencyManager::get<UserInputMapper>()->getActionState(UserInputMapper::Action(action));
    }

    int ScriptingInterface::findAction(QString actionName) {
        return DependencyManager::get<UserInputMapper>()->findAction(actionName);
    }

    QVector<QString> ScriptingInterface::getActionNames() const {
        return DependencyManager::get<UserInputMapper>()->getActionNames();
    }

} // namespace controllers


using namespace controller;
// FIXME this throws a hissy fit on MSVC if I put it in the main controller namespace block
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
        UserInputMapper::Input actionInput(UserInputMapper::ACTIONS_DEVICE, actionNumber++, UserInputMapper::ChannelType::AXIS);
        qCDebug(controllers) << "\tAction: " << actionName << " " << QString::number(actionInput.getID(), 16);
        // Expose the IDs to JS
        QString cleanActionName = QString(actionName).remove(ScriptingInterface::SANITIZE_NAME_EXPRESSION);
        _actions.insert(cleanActionName, actionInput.getID());

        // Create the action endpoints
        _endpoints[actionInput] = std::make_shared<ActionEndpoint>(actionInput);
    }

    updateMaps();
}
