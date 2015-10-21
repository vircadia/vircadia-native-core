//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UserInputMapper.h"

#include <set>

#include <QtCore/QThread>
#include <QtCore/QFile>

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>

#include <PathUtils.h>

#include "StandardController.h"
#include "Logging.h"

namespace controller {
    const uint16_t UserInputMapper::ACTIONS_DEVICE = Input::INVALID_DEVICE - 0xFF;
    const uint16_t UserInputMapper::STANDARD_DEVICE = 0;
}

// Default contruct allocate the poutput size with the current hardcoded action channels
controller::UserInputMapper::UserInputMapper() {
    _activeMappings.push_back(_defaultMapping);
    _standardController = std::make_shared<StandardController>();
    registerDevice(new ActionsDevice());
    registerDevice(_standardController.get());
}

namespace controller {

class ScriptEndpoint : public Endpoint {
    Q_OBJECT;
public:
    ScriptEndpoint(const QScriptValue& callable)
        : Endpoint(Input::INVALID_INPUT), _callable(callable) {
    }

    virtual float value();
    virtual void apply(float newValue, float oldValue, const Pointer& source);

protected:
    Q_INVOKABLE void updateValue();
    Q_INVOKABLE virtual void internalApply(float newValue, float oldValue, int sourceID);
private:
    QScriptValue _callable;
    float _lastValue = 0.0f;
};

class VirtualEndpoint : public Endpoint {
public:
    VirtualEndpoint(const Input& id = Input::INVALID_INPUT)
        : Endpoint(id) {
    }

    virtual float value() override { return _currentValue; }
    virtual void apply(float newValue, float oldValue, const Pointer& source) override { _currentValue = newValue; }

    virtual Pose pose() override { return _currentPose; }
    virtual void apply(const Pose& newValue, const Pose& oldValue, const Pointer& source) override {
        _currentPose = newValue;
    }
private:
    float _currentValue{ 0.0f };
    Pose _currentPose{};
};


class JSEndpoint : public Endpoint {
public:
    JSEndpoint(const QJSValue& callable)
        : Endpoint(Input::INVALID_INPUT), _callable(callable) {
    }

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
        : Endpoint(Input::INVALID_INPUT), Pair(first, second) { }

    virtual float value() {
        float result = first->value() * -1.0f + second->value();
        return result;
    }

    virtual void apply(float newValue, float oldValue, const Pointer& source) {
        // Composites are read only
    }

private:
    Endpoint::Pointer _first;
    Endpoint::Pointer _second;
};


class InputEndpoint : public Endpoint {
public:
    InputEndpoint(const Input& id = Input::INVALID_INPUT)
        : Endpoint(id) {
    }

    virtual float value() override {
        _currentValue = 0.0f;
        if (isPose()) {
            return _currentValue;
        }
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        auto deviceProxy = userInputMapper->getDeviceProxy(_input);
        if (!deviceProxy) {
            return _currentValue;
        }
        _currentValue = deviceProxy->getValue(_input, 0);
        return _currentValue;
    }
    virtual void apply(float newValue, float oldValue, const Pointer& source) override {}

    virtual Pose pose() override {
        _currentPose = Pose();
        if (!isPose()) {
            return _currentPose;
        }
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        auto deviceProxy = userInputMapper->getDeviceProxy(_input);
        if (!deviceProxy) {
            return _currentPose;
        }
        _currentPose = deviceProxy->getPose(_input, 0);
        return _currentPose;
    }

    virtual void apply(const Pose& newValue, const Pose& oldValue, const Pointer& source) override {
    }

private:
    float _currentValue{ 0.0f };
    Pose _currentPose{};
};

class ActionEndpoint : public Endpoint {
public:
    ActionEndpoint(const Input& id = Input::INVALID_INPUT)
        : Endpoint(id) {
    }

    virtual float value() override { return _currentValue; }
    virtual void apply(float newValue, float oldValue, const Pointer& source) override {

        _currentValue += newValue;
        if (!(_input == Input::INVALID_INPUT)) {
            auto userInputMapper = DependencyManager::get<UserInputMapper>();
            userInputMapper->deltaActionState(Action(_input.getChannel()), newValue);
        }
    }

    virtual Pose pose() override { return _currentPose; }
    virtual void apply(const Pose& newValue, const Pose& oldValue, const Pointer& source) override {
        _currentPose = newValue;
        if (!_currentPose.isValid()) {
            return;
        }
        if (!(_input == Input::INVALID_INPUT)) {
            auto userInputMapper = DependencyManager::get<UserInputMapper>();
            userInputMapper->setActionState(Action(_input.getChannel()), _currentPose);
        }
    }

private:
    float _currentValue{ 0.0f };
    Pose _currentPose{};
};

UserInputMapper::~UserInputMapper() {
}

int UserInputMapper::recordDeviceOfType(const QString& deviceName) {
    if (!_deviceCounts.contains(deviceName)) {
        _deviceCounts[deviceName] = 0;
    }
    _deviceCounts[deviceName] += 1;
    return _deviceCounts[deviceName];
}

void UserInputMapper::registerDevice(InputDevice* device) {
    if (device->_deviceID == Input::INVALID_DEVICE) {
        device->_deviceID = getFreeDeviceID();
    }
    const auto& deviceID = device->_deviceID;
    DeviceProxy::Pointer proxy = std::make_shared<DeviceProxy>();
    proxy->_name = device->_name;
    device->buildDeviceProxy(proxy);

    int numberOfType = recordDeviceOfType(proxy->_name);
    if (numberOfType > 1) {
        proxy->_name += QString::number(numberOfType);
    }

    qCDebug(controllers) << "Registered input device <" << proxy->_name << "> deviceID = " << deviceID;

    for (const auto& inputMapping : proxy->getAvailabeInputs()) {
        const auto& input = inputMapping.first;
        // Ignore aliases
        if (_endpointsByInput.count(input)) {
            continue;
        }
        Endpoint::Pointer endpoint;
        if (input.device == STANDARD_DEVICE) {
             endpoint = std::make_shared<VirtualEndpoint>(input);
        } else if (input.device == ACTIONS_DEVICE) {
            endpoint = std::make_shared<ActionEndpoint>(input);
        } else {
            endpoint = std::make_shared<InputEndpoint>(input);
        }
        _inputsByEndpoint[endpoint] = input;
        _endpointsByInput[input] = endpoint;
    }

    _registeredDevices[deviceID] = proxy;
    auto mapping = loadMapping(device->getDefaultMappingConfig());
    if (mapping) {
        _mappingsByDevice[deviceID] = mapping;
        auto& defaultRoutes = _defaultMapping->routes;

        // New routes for a device get injected IN FRONT of existing routes.  Routes
        // are processed in order so this ensures that the standard -> action processing 
        // takes place after all of the hardware -> standard or hardware -> action processing
        // because standard -> action is the first set of routes added.
        defaultRoutes.insert(defaultRoutes.begin(), mapping->routes.begin(), mapping->routes.end());
    }

    emit hardwareChanged();
}

// FIXME remove the associated device mappings
void UserInputMapper::removeDevice(int deviceID) {
    auto proxyEntry = _registeredDevices.find(deviceID);
    if (_registeredDevices.end() == proxyEntry) {
        qCWarning(controllers) << "Attempted to remove unknown device " << deviceID;
        return;
    }
    auto proxy = proxyEntry->second;
    auto mappingsEntry = _mappingsByDevice.find(deviceID);
    if (_mappingsByDevice.end() != mappingsEntry) {
        const auto& mapping = mappingsEntry->second;
        const auto& deviceRoutes = mapping->routes;
        std::set<Route::Pointer> routeSet(deviceRoutes.begin(), deviceRoutes.end());

        auto& defaultRoutes = _defaultMapping->routes;
        std::remove_if(defaultRoutes.begin(), defaultRoutes.end(), [&](Route::Pointer route)->bool {
            return routeSet.count(route) != 0;
        });

        _mappingsByDevice.erase(mappingsEntry);
    }

    _registeredDevices.erase(proxyEntry);

    emit hardwareChanged();
}


DeviceProxy::Pointer UserInputMapper::getDeviceProxy(const Input& input) {
    auto device = _registeredDevices.find(input.getDevice());
    if (device != _registeredDevices.end()) {
        return (device->second);
    } else {
        return DeviceProxy::Pointer();
    }
}

QString UserInputMapper::getDeviceName(uint16 deviceID) { 
    if (_registeredDevices.find(deviceID) != _registeredDevices.end()) {
        return _registeredDevices[deviceID]->_name;
    }
    return QString("unknown");
}

int UserInputMapper::findDevice(QString name) const {
    for (auto device : _registeredDevices) {
        if (device.second->_name == name) {
            return device.first;
        }
    }
    return Input::INVALID_DEVICE;
}

QVector<QString> UserInputMapper::getDeviceNames() {
    QVector<QString> result;
    for (auto device : _registeredDevices) {
        QString deviceName = device.second->_name.split(" (")[0];
        result << deviceName;
    }
    return result;
}

int UserInputMapper::findAction(const QString& actionName) const {
    return findDeviceInput("Actions." + actionName).getChannel();
}

Input UserInputMapper::findDeviceInput(const QString& inputName) const {
    // Split the full input name as such: deviceName.inputName
    auto names = inputName.split('.');

    if (names.size() >= 2) {
        // Get the device name:
        auto deviceName = names[0];
        auto inputName = names[1];

        int deviceID = findDevice(deviceName);
        if (deviceID != Input::INVALID_DEVICE) {
            const auto& deviceProxy = _registeredDevices.at(deviceID);
            auto deviceInputs = deviceProxy->getAvailabeInputs();

            for (auto input : deviceInputs) {
                if (input.second == inputName) {
                    return input.first;
                }
            }

            qCDebug(controllers) << "Couldn\'t find InputChannel named <" << inputName << "> for device <" << deviceName << ">";

        } else {
            qCDebug(controllers) << "Couldn\'t find InputDevice named <" << deviceName << ">";
            findDevice(deviceName);
        }
    } else {
        qCDebug(controllers) << "Couldn\'t understand <" << inputName << "> as a valid inputDevice.inputName";
    }

    return Input::INVALID_INPUT;
}

void fixBisectedAxis(float& full, float& negative, float& positive) {
    full = full + (negative * -1.0f) + positive;
    negative = full >= 0.0f ? 0.0f : full * -1.0f;
    positive = full <= 0.0f ? 0.0f : full;
}

void UserInputMapper::update(float deltaTime) {
    // Reset the axis state for next loop
    for (auto& channel : _actionStates) {
        channel = 0.0f;
    }
    
    for (auto& channel : _poseStates) {
        channel = Pose();
    }

    // Run the mappings code
    update();

    // Scale all the channel step with the scale
    for (auto i = 0; i < toInt(Action::NUM_ACTIONS); i++) {
        if (_externalActionStates[i] != 0) {
            _actionStates[i] += _externalActionStates[i];
            _externalActionStates[i] = 0.0f;
        }

        if (_externalPoseStates[i].isValid()) {
            _poseStates[i] = _externalPoseStates[i];
            _externalPoseStates[i] = Pose();
        }
    }

    // merge the bisected and non-bisected axes for now
    fixBisectedAxis(_actionStates[toInt(Action::TRANSLATE_X)], _actionStates[toInt(Action::LATERAL_LEFT)], _actionStates[toInt(Action::LATERAL_RIGHT)]);
    fixBisectedAxis(_actionStates[toInt(Action::TRANSLATE_Y)], _actionStates[toInt(Action::VERTICAL_DOWN)], _actionStates[toInt(Action::VERTICAL_UP)]);
    fixBisectedAxis(_actionStates[toInt(Action::TRANSLATE_Z)], _actionStates[toInt(Action::LONGITUDINAL_FORWARD)], _actionStates[toInt(Action::LONGITUDINAL_BACKWARD)]);
    fixBisectedAxis(_actionStates[toInt(Action::TRANSLATE_CAMERA_Z)], _actionStates[toInt(Action::BOOM_IN)], _actionStates[toInt(Action::BOOM_OUT)]);
    fixBisectedAxis(_actionStates[toInt(Action::ROTATE_Y)], _actionStates[toInt(Action::YAW_LEFT)], _actionStates[toInt(Action::YAW_RIGHT)]);
    fixBisectedAxis(_actionStates[toInt(Action::ROTATE_X)], _actionStates[toInt(Action::PITCH_UP)], _actionStates[toInt(Action::PITCH_DOWN)]);


    static const float EPSILON = 0.01f;
    for (auto i = 0; i < toInt(Action::NUM_ACTIONS); i++) {
        _actionStates[i] *= _actionScales[i];
        // Emit only on change, and emit when moving back to 0
        if (fabsf(_actionStates[i] - _lastActionStates[i]) > EPSILON) {
            _lastActionStates[i] = _actionStates[i];
            emit actionEvent(i, _actionStates[i]);
        }
        // TODO: emit signal for pose changes
    }
}

Input::NamedVector UserInputMapper::getAvailableInputs(uint16 deviceID) const {
    auto iterator = _registeredDevices.find(deviceID);
    return iterator->second->getAvailabeInputs();
}

QVector<Action> UserInputMapper::getAllActions() const {
    QVector<Action> actions;
    for (auto i = 0; i < toInt(Action::NUM_ACTIONS); i++) {
        actions.append(Action(i));
    }
    return actions;
}

QString UserInputMapper::getActionName(Action action) const {
    for (auto actionPair : getActionInputs()) {
        if (actionPair.first.channel == toInt(action)) {
            return actionPair.second;
        }
    }
    return QString();
}


QVector<QString> UserInputMapper::getActionNames() const {
    QVector<QString> result;
    for (auto actionPair : getActionInputs()) {
        result << actionPair.second;
    }
    return result;
}
/*
void UserInputMapper::assignDefaulActionScales() {
    _actionScales[toInt(Action::LONGITUDINAL_BACKWARD)] = 1.0f; // 1m per unit
    _actionScales[toInt(Action::LONGITUDINAL_FORWARD)] = 1.0f; // 1m per unit
    _actionScales[toInt(Action::LATERAL_LEFT)] = 1.0f; // 1m per unit
    _actionScales[toInt(Action::LATERAL_RIGHT)] = 1.0f; // 1m per unit
    _actionScales[toInt(Action::VERTICAL_DOWN)] = 1.0f; // 1m per unit
    _actionScales[toInt(Action::VERTICAL_UP)] = 1.0f; // 1m per unit
    _actionScales[toInt(Action::YAW_LEFT)] = 1.0f; // 1 degree per unit
    _actionScales[toInt(Action::YAW_RIGHT)] = 1.0f; // 1 degree per unit
    _actionScales[toInt(Action::PITCH_DOWN)] = 1.0f; // 1 degree per unit
    _actionScales[toInt(Action::PITCH_UP)] = 1.0f; // 1 degree per unit
    _actionScales[toInt(Action::BOOM_IN)] = 0.5f; // .5m per unit
    _actionScales[toInt(Action::BOOM_OUT)] = 0.5f; // .5m per unit
    _actionScales[toInt(Action::LEFT_HAND)] = 1.0f; // default
    _actionScales[toInt(Action::RIGHT_HAND)] = 1.0f; // default
    _actionScales[toInt(Action::LEFT_HAND_CLICK)] = 1.0f; // on
    _actionScales[toInt(Action::RIGHT_HAND_CLICK)] = 1.0f; // on
    _actionScales[toInt(Action::SHIFT)] = 1.0f; // on
    _actionScales[toInt(Action::ACTION1)] = 1.0f; // default
    _actionScales[toInt(Action::ACTION2)] = 1.0f; // default
    _actionScales[toInt(Action::TRANSLATE_X)] = 1.0f; // default
    _actionScales[toInt(Action::TRANSLATE_Y)] = 1.0f; // default
    _actionScales[toInt(Action::TRANSLATE_Z)] = 1.0f; // default
    _actionScales[toInt(Action::ROLL)] = 1.0f; // default
    _actionScales[toInt(Action::PITCH)] = 1.0f; // default
    _actionScales[toInt(Action::YAW)] = 1.0f; // default
}
*/

static int actionMetaTypeId = qRegisterMetaType<Action>();
static int inputMetaTypeId = qRegisterMetaType<Input>();
static int inputPairMetaTypeId = qRegisterMetaType<InputPair>();
static int poseMetaTypeId = qRegisterMetaType<controller::Pose>("Pose");


QScriptValue inputToScriptValue(QScriptEngine* engine, const Input& input);
void inputFromScriptValue(const QScriptValue& object, Input& input);
QScriptValue actionToScriptValue(QScriptEngine* engine, const Action& action);
void actionFromScriptValue(const QScriptValue& object, Action& action);
QScriptValue inputPairToScriptValue(QScriptEngine* engine, const InputPair& inputPair);
void inputPairFromScriptValue(const QScriptValue& object, InputPair& inputPair);

QScriptValue inputToScriptValue(QScriptEngine* engine, const Input& input) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("device", input.getDevice());
    obj.setProperty("channel", input.getChannel());
    obj.setProperty("type", (unsigned short)input.getType());
    obj.setProperty("id", input.getID());
    return obj;
}

void inputFromScriptValue(const QScriptValue& object, Input& input) {
    input.id = object.property("id").toInt32();
}

QScriptValue actionToScriptValue(QScriptEngine* engine, const Action& action) {
    QScriptValue obj = engine->newObject();
    auto userInputMapper = DependencyManager::get<UserInputMapper>();
    obj.setProperty("action", (int)action);
    obj.setProperty("actionName", userInputMapper->getActionName(action));
    return obj;
}

void actionFromScriptValue(const QScriptValue& object, Action& action) {
    action = Action(object.property("action").toVariant().toInt());
}

QScriptValue inputPairToScriptValue(QScriptEngine* engine, const InputPair& inputPair) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("input", inputToScriptValue(engine, inputPair.first));
    obj.setProperty("inputName", inputPair.second);
    return obj;
}

void inputPairFromScriptValue(const QScriptValue& object, InputPair& inputPair) {
    inputFromScriptValue(object.property("input"), inputPair.first);
    inputPair.second = QString(object.property("inputName").toVariant().toString());
}

void UserInputMapper::registerControllerTypes(QScriptEngine* engine) {
    qScriptRegisterSequenceMetaType<QVector<Action> >(engine);
    qScriptRegisterSequenceMetaType<QVector<InputPair> >(engine);
    qScriptRegisterMetaType(engine, actionToScriptValue, actionFromScriptValue);
    qScriptRegisterMetaType(engine, inputToScriptValue, inputFromScriptValue);
    qScriptRegisterMetaType(engine, inputPairToScriptValue, inputPairFromScriptValue);

    qScriptRegisterMetaType(engine, Pose::toScriptValue, Pose::fromScriptValue);
}

Input UserInputMapper::makeStandardInput(controller::StandardButtonChannel button) {
    return Input(STANDARD_DEVICE, button, ChannelType::BUTTON);
}

Input UserInputMapper::makeStandardInput(controller::StandardAxisChannel axis) {
    return Input(STANDARD_DEVICE, axis, ChannelType::AXIS);
}

Input UserInputMapper::makeStandardInput(controller::StandardPoseChannel pose) {
    return Input(STANDARD_DEVICE, pose, ChannelType::POSE);
}

void UserInputMapper::update() {
    static auto deviceNames = getDeviceNames();
    _overrideValues.clear();

    EndpointSet readEndpoints;
    EndpointSet writtenEndpoints;

    // Now process the current values for each level of the stack
    for (auto& mapping : _activeMappings) {
        for (const auto& route : mapping->routes) {
            const auto& source = route->source;
            // Endpoints can only be read once (though a given mapping can route them to 
            // multiple places).  Consider... If the default is to wire the A button to JUMP
            // and someone else wires it to CONTEXT_MENU, I don't want both to occur when 
            // I press the button.  The exception is if I'm wiring a control back to itself
            // in order to adjust my interface, like inverting the Y axis on an analog stick
            if (readEndpoints.count(source)) {
                continue;
            }

            const auto& destination = route->destination;
            // THis could happen if the route destination failed to create
            // FIXME: Maybe do not create the route if the destination failed and avoid this case ?
            if (!destination) {
                continue;
            }

            if (writtenEndpoints.count(destination)) {
                continue;
            }

            if (route->conditional) {
                if (!route->conditional->satisfied()) {
                    continue;
                }
            }

            // Standard controller destinations can only be can only be used once.
            if (getStandardDeviceID() == destination->getInput().getDevice()) {
                writtenEndpoints.insert(destination);
            }

            // Only consume the input if the route isn't a loopback.
            // This allows mappings like `mapping.from(xbox.RY).invert().to(xbox.RY);`
            bool loopback = source == destination;
            if (!loopback) {
                readEndpoints.insert(source);
            }

            // Fetch the value, may have been overriden by previous loopback routes
            if (source->isPose()) {
                Pose value = getPose(source);
                // no filters yet for pose
                destination->apply(value, Pose(), source);
            } else {
                // Fetch the value, may have been overriden by previous loopback routes
                float value = getValue(source);

                // Apply each of the filters.
                for (const auto& filter : route->filters) {
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

Endpoint::Pointer UserInputMapper::endpointFor(const QJSValue& endpoint) {
    if (endpoint.isNumber()) {
        return endpointFor(Input(endpoint.toInt()));
    }

    if (endpoint.isCallable()) {
        auto result = std::make_shared<JSEndpoint>(endpoint);
        return result;
    }

    qWarning() << "Unsupported input type " << endpoint.toString();
    return Endpoint::Pointer();
}

Endpoint::Pointer UserInputMapper::endpointFor(const QScriptValue& endpoint) {
    if (endpoint.isNumber()) {
        return endpointFor(Input(endpoint.toInt32()));
    }

    if (endpoint.isFunction()) {
        auto result = std::make_shared<ScriptEndpoint>(endpoint);
        return result;
    }

    qWarning() << "Unsupported input type " << endpoint.toString();
    return Endpoint::Pointer();
}

Endpoint::Pointer UserInputMapper::endpointFor(const Input& inputId) const {
    auto iterator = _endpointsByInput.find(inputId);
    if (_endpointsByInput.end() == iterator) {
        qWarning() << "Unknown input: " << QString::number(inputId.getID(), 16);
        return Endpoint::Pointer();
    }
    return iterator->second;
}

Endpoint::Pointer UserInputMapper::compositeEndpointFor(Endpoint::Pointer first, Endpoint::Pointer second) {
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


Mapping::Pointer UserInputMapper::newMapping(const QString& mappingName) {
    if (_mappingsByName.count(mappingName)) {
        qCWarning(controllers) << "Refusing to recreate mapping named " << mappingName;
    }
    qDebug() << "Creating new Mapping " << mappingName;
    auto mapping = std::make_shared<Mapping>(mappingName);
    _mappingsByName[mappingName] = mapping;
    return mapping;
}

// FIXME handle asynchronous loading in the UserInputMapper
//QObject* ScriptingInterface::loadMapping(const QString& jsonUrl) {
//    QObject* result = nullptr;
//    auto request = ResourceManager::createResourceRequest(nullptr, QUrl(jsonUrl));
//    if (request) {
//        QEventLoop eventLoop;
//        request->setCacheEnabled(false);
//        connect(request, &ResourceRequest::finished, &eventLoop, &QEventLoop::quit);
//        request->send();
//        if (request->getState() != ResourceRequest::Finished) {
//            eventLoop.exec();
//        }
//
//        if (request->getResult() == ResourceRequest::Success) {
//            result = parseMapping(QString(request->getData()));
//        } else {
//            qCWarning(controllers) << "Failed to load mapping url <" << jsonUrl << ">" << endl;
//        }
//        request->deleteLater();
//    }
//    return result;
//}


void UserInputMapper::enableMapping(const QString& mappingName, bool enable) {
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

float UserInputMapper::getValue(const Endpoint::Pointer& endpoint) const {
    auto valuesIterator = _overrideValues.find(endpoint);
    if (_overrideValues.end() != valuesIterator) {
        return valuesIterator->second;
    }

    return endpoint->value();
}

float UserInputMapper::getValue(const Input& input) const {
    auto endpoint = endpointFor(input);
    if (!endpoint) {
        return 0;
    }
    return endpoint->value();
}

Pose UserInputMapper::getPose(const Endpoint::Pointer& endpoint) const {
    if (!endpoint->isPose()) {
        return Pose();
    }
    return endpoint->pose();
}

Pose UserInputMapper::getPose(const Input& input) const {
    auto endpoint = endpointFor(input);
    if (!endpoint) {
        return Pose();
    }
    return getPose(endpoint);
}

Mapping::Pointer UserInputMapper::loadMapping(const QString& jsonFile) {
    if (jsonFile.isEmpty()) {
        return Mapping::Pointer();
    }
    QString json;
    {
        QFile file(jsonFile);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            json = QTextStream(&file).readAll();
        }
        file.close();
    }
    return parseMapping(json);
}


const QString JSON_NAME = QStringLiteral("name");
const QString JSON_CHANNELS = QStringLiteral("channels");
const QString JSON_CHANNEL_FROM = QStringLiteral("from");
const QString JSON_CHANNEL_WHEN = QStringLiteral("when");
const QString JSON_CHANNEL_TO = QStringLiteral("to");
const QString JSON_CHANNEL_FILTERS = QStringLiteral("filters");

Endpoint::Pointer UserInputMapper::parseEndpoint(const QJsonValue& value) {
    if (value.isString()) {
        auto input = findDeviceInput(value.toString());
        return endpointFor(input);
    } else if (value.isObject()) {
        // Endpoint is defined as an object, we expect a js function then
        return Endpoint::Pointer();
    }
    return Endpoint::Pointer();
}

Conditional::Pointer UserInputMapper::parseConditional(const QJsonValue& value) {
    if (value.isString()) {
        auto input = findDeviceInput(value.toString());
        auto endpoint = endpointFor(input);
        if (!endpoint) {
            return Conditional::Pointer();
        }

        return std::make_shared<EndpointConditional>(endpoint);
    } 
      
    return Conditional::parse(value);
}

Route::Pointer UserInputMapper::parseRoute(const QJsonValue& value) {
    if (!value.isObject()) {
        return Route::Pointer();
    }

    const auto& obj = value.toObject();
    Route::Pointer result = std::make_shared<Route>();
    result->source = parseEndpoint(obj[JSON_CHANNEL_FROM]);
    if (!result->source) {
        qWarning() << "Invalid route source " << obj[JSON_CHANNEL_FROM];
        return Route::Pointer();
    }
    result->destination = parseEndpoint(obj[JSON_CHANNEL_TO]);
    if (!result->destination) {
        qWarning() << "Invalid route destination " << obj[JSON_CHANNEL_TO];
        return Route::Pointer();
    }

    if (obj.contains(JSON_CHANNEL_WHEN)) {
        auto when = parseConditional(obj[JSON_CHANNEL_WHEN]);
        if (!when) {
            qWarning() << "Invalid route conditional " << obj[JSON_CHANNEL_TO];
            return Route::Pointer();
        }
        result->conditional = when;
    }

    const auto& filtersValue = obj[JSON_CHANNEL_FILTERS];
    // FIXME support strings for filters with no parameters, both in the array and at the top level...
    // i.e.
    // { "from": "Standard.DU", "to" : "Actions.LONGITUDINAL_FORWARD", "filters" : "invert" },
    // and 
    // { "from": "Standard.DU", "to" : "Actions.LONGITUDINAL_FORWARD", "filters" : [ "invert", "constrainToInteger" ] },
    if (filtersValue.isArray()) {
        auto filtersArray = filtersValue.toArray();
        for (auto filterValue : filtersArray) {
            if (!filterValue.isObject()) {
                qWarning() << "Invalid filter " << filterValue;
                return Route::Pointer();
            }
            Filter::Pointer filter = Filter::parse(filterValue.toObject());
            if (!filter) {
                qWarning() << "Invalid filter " << filterValue;
                return Route::Pointer();
            }
            result->filters.push_back(filter);
        }
    }
    return result;
}

Mapping::Pointer UserInputMapper::parseMapping(const QJsonValue& json) {
    if (!json.isObject()) {
        return Mapping::Pointer();
    }

    auto obj = json.toObject();
    auto mapping = std::make_shared<Mapping>("default");
    mapping->name = obj[JSON_NAME].toString();
    const auto& jsonChannels = obj[JSON_CHANNELS].toArray();
    for (const auto& channelIt : jsonChannels) {
        Route::Pointer route = parseRoute(channelIt);
        if (!route) {
            qWarning() << "Couldn't parse route";
            continue;
        }
        mapping->routes.push_back(route);
    }
    return mapping;
}

Mapping::Pointer UserInputMapper::parseMapping(const QString& json) {
    Mapping::Pointer result;
    QJsonObject obj;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    // check validity of the document
    if (doc.isNull()) {
        return Mapping::Pointer();
    }

    if (!doc.isObject()) {
        qWarning() << "Mapping json Document is not an object" << endl;
        return Mapping::Pointer();
    }

    // FIXME how did we detect this?
    //    qDebug() << "Invalid JSON...\n";
    //    qDebug() << error.errorString();
    //    qDebug() << "JSON was:\n" << json << endl;
    //}
    return parseMapping(doc.object());
}

}

#include "UserInputMapper.moc"
