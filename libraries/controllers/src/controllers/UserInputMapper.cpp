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

class StandardEndpoint : public VirtualEndpoint {
public:
    StandardEndpoint(const Input& input) : VirtualEndpoint(input) {}
    virtual bool writeable() const override { return !_written; }
    virtual bool readable() const override { return !_read; }
    virtual void reset() override { 
        apply(0.0f, 0.0f, Endpoint::Pointer());
        apply(Pose(), Pose(), Endpoint::Pointer());
        _written = _read = false;
    }

    virtual float value() override { 
        _read = true;
        return VirtualEndpoint::value();
    }

    virtual void apply(float newValue, float oldValue, const Pointer& source) override { 
        // For standard endpoints, the first NON-ZERO write counts.  
        if (newValue != 0.0) {
            _written = true;
        }
        VirtualEndpoint::apply(newValue, oldValue, source);
    }

    virtual Pose pose() override { 
        _read = true;
        return VirtualEndpoint::pose();
    }

    virtual void apply(const Pose& newValue, const Pose& oldValue, const Pointer& source) override {
        if (newValue != Pose()) {
            _written = true;
        }
        VirtualEndpoint::apply(newValue, oldValue, source);
    }

private:
    bool _written { false };
    bool _read { false };
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
};

class ArrayEndpoint : public Endpoint {
    friend class UserInputMapper;
public:
    using Pointer = std::shared_ptr<ArrayEndpoint>;
    ArrayEndpoint() : Endpoint(Input::INVALID_INPUT) { }

    virtual float value() override {
        return 0.0;
    }

    virtual void apply(float newValue, float oldValue, const Endpoint::Pointer& source) override {
        for (auto& child : _children) {
            if (child->writeable()) {
                child->apply(newValue, oldValue, source);
            }
        }
    }

    virtual bool readable() const override { return false; }

private:
    Endpoint::List _children;
};

class AnyEndpoint : public Endpoint {
    friend class UserInputMapper;
public:
    using Pointer = std::shared_ptr<AnyEndpoint>;
    AnyEndpoint() : Endpoint(Input::INVALID_INPUT) {}

    virtual float value() override {
        float result = 0;
        for (auto& child : _children) {
            float childResult = child->value();
            if (childResult != 0.0f) {
                result = childResult;
            }
        }
        return result;
    }

    virtual void apply(float newValue, float oldValue, const Endpoint::Pointer& source) override {
        qFatal("AnyEndpoint is read only");
    }

    virtual bool writeable() const override { return false; }

    virtual bool readable() const override { 
        for (auto& child : _children) {
            if (!child->readable()) {
                return false;
            }
        }
        return true;
    }

private:
    Endpoint::List _children;
};

class InputEndpoint : public Endpoint {
public:
    InputEndpoint(const Input& id = Input::INVALID_INPUT)
        : Endpoint(id) {
    }

    virtual float value() override {
        _read = true;
        if (isPose()) {
            return pose().valid ? 1.0f : 0.0f;
        }
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        auto deviceProxy = userInputMapper->getDeviceProxy(_input);
        if (!deviceProxy) {
            return 0.0f;
        }
        return deviceProxy->getValue(_input, 0);
    }

    // FIXME need support for writing back to vibration / force feedback effects
    virtual void apply(float newValue, float oldValue, const Pointer& source) override {}

    virtual Pose pose() override {
        _read = true;
        if (!isPose()) {
            return Pose();
        }
        auto userInputMapper = DependencyManager::get<UserInputMapper>();
        auto deviceProxy = userInputMapper->getDeviceProxy(_input);
        if (!deviceProxy) {
            return Pose();
        }
        return deviceProxy->getPose(_input, 0);
    }

    virtual void apply(const Pose& newValue, const Pose& oldValue, const Pointer& source) override { }

    virtual bool writeable() const { return !_written; }
    virtual bool readable() const { return !_read; }
    virtual void reset() { _written = _read = false; }

private:

    bool _written { false };
    bool _read { false };
};

class ActionEndpoint : public Endpoint {
public:
    ActionEndpoint(const Input& id = Input::INVALID_INPUT)
        : Endpoint(id) {
    }

    virtual float value() override { return _currentValue; }
    virtual void apply(float newValue, float oldValue, const Pointer& source) override {
        _currentValue += newValue;
        if (_input != Input::INVALID_INPUT) {
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
        if (_input != Input::INVALID_INPUT) {
            auto userInputMapper = DependencyManager::get<UserInputMapper>();
            userInputMapper->setActionState(Action(_input.getChannel()), _currentPose);
        }
    }

    virtual void reset() override {
        _currentValue = 0.0f;
        _currentPose = Pose();
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
    Locker locker(_lock);
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
            endpoint = std::make_shared<StandardEndpoint>(input);
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
        enableMapping(mapping);
    }

    emit hardwareChanged();
}

// FIXME remove the associated device mappings
void UserInputMapper::removeDevice(int deviceID) {
    Locker locker(_lock);
    auto proxyEntry = _registeredDevices.find(deviceID);
    if (_registeredDevices.end() == proxyEntry) {
        qCWarning(controllers) << "Attempted to remove unknown device " << deviceID;
        return;
    }
    auto proxy = proxyEntry->second;
    auto mappingsEntry = _mappingsByDevice.find(deviceID);
    if (_mappingsByDevice.end() != mappingsEntry) {
        disableMapping(mappingsEntry->second);
        _mappingsByDevice.erase(mappingsEntry);
    }

    _registeredDevices.erase(proxyEntry);

    emit hardwareChanged();
}


DeviceProxy::Pointer UserInputMapper::getDeviceProxy(const Input& input) {
    Locker locker(_lock);
    auto device = _registeredDevices.find(input.getDevice());
    if (device != _registeredDevices.end()) {
        return (device->second);
    } else {
        return DeviceProxy::Pointer();
    }
}

QString UserInputMapper::getDeviceName(uint16 deviceID) { 
    Locker locker(_lock);
    if (_registeredDevices.find(deviceID) != _registeredDevices.end()) {
        return _registeredDevices[deviceID]->_name;
    }
    return QString("unknown");
}

int UserInputMapper::findDevice(QString name) const {
    Locker locker(_lock);
    for (auto device : _registeredDevices) {
        if (device.second->_name == name) {
            return device.first;
        }
    }
    return Input::INVALID_DEVICE;
}

QVector<QString> UserInputMapper::getDeviceNames() {
    Locker locker(_lock);
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
    Locker locker(_lock);
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
    Locker locker(_lock);
    // Reset the axis state for next loop
    for (auto& channel : _actionStates) {
        channel = 0.0f;
    }
    
    for (auto& channel : _poseStates) {
        channel = Pose();
    }

    // Run the mappings code
    runMappings();

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
    Locker locker(_lock);
    auto iterator = _registeredDevices.find(deviceID);
    return iterator->second->getAvailabeInputs();
}

QVector<Action> UserInputMapper::getAllActions() const {
    Locker locker(_lock);
    QVector<Action> actions;
    for (auto i = 0; i < toInt(Action::NUM_ACTIONS); i++) {
        actions.append(Action(i));
    }
    return actions;
}

QString UserInputMapper::getActionName(Action action) const {
    Locker locker(_lock);
    for (auto actionPair : getActionInputs()) {
        if (actionPair.first.channel == toInt(action)) {
            return actionPair.second;
        }
    }
    return QString();
}


QVector<QString> UserInputMapper::getActionNames() const {
    Locker locker(_lock);
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

void UserInputMapper::runMappings() {
    static auto deviceNames = getDeviceNames();
    _overrides.clear();

    for (auto endpointEntry : this->_endpointsByInput) {
        endpointEntry.second->reset();
    }

    // Now process the current values for each level of the stack
    for (const auto& route : _deviceRoutes) {
        if (!route) {
            continue;
        }
        applyRoute(route);
    }

    for (const auto& route : _standardRoutes) {
        if (!route) {
            continue;
        }
        applyRoute(route);
    }

}


void UserInputMapper::applyRoute(const Route::Pointer& route) {
    if (route->conditional) {
        if (!route->conditional->satisfied()) {
            return;
        }
    }

    auto source = route->source;
    if (_overrides.count(source)) {
        source = _overrides[source];
    }

    // Endpoints can only be read once (though a given mapping can route them to 
    // multiple places).  Consider... If the default is to wire the A button to JUMP
    // and someone else wires it to CONTEXT_MENU, I don't want both to occur when 
    // I press the button.  The exception is if I'm wiring a control back to itself
    // in order to adjust my interface, like inverting the Y axis on an analog stick
    if (!source->readable()) {
        return;
    }


    auto input = source->getInput();
    float value = source->value();
    if (value != 0.0) {
        int i = 0;
    }

    auto destination = route->destination;
    // THis could happen if the route destination failed to create
    // FIXME: Maybe do not create the route if the destination failed and avoid this case ?
    if (!destination) {
        return;
    }

    // FIXME?, should come before or after the override logic?
    if (!destination->writeable()) {
        return;
    }

    // Only consume the input if the route isn't a loopback.
    // This allows mappings like `mapping.from(xbox.RY).invert().to(xbox.RY);`
    bool loopback = (source->getInput() == destination->getInput()) && (source->getInput() != Input::INVALID_INPUT);
    // Each time we loop back we re-write the override 
    if (loopback) {
        _overrides[source] = destination = std::make_shared<StandardEndpoint>(source->getInput());
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

        destination->apply(value, 0, source);
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
    Locker locker(_lock);
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
    Locker locker(_lock);
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
    Locker locker(_lock);
    qCDebug(controllers) << "Attempting to enable mapping " << mappingName;
    auto iterator = _mappingsByName.find(mappingName);
    if (_mappingsByName.end() == iterator) {
        qCWarning(controllers) << "Request to enable / disable unknown mapping " << mappingName;
        return;
    }

    auto mapping = iterator->second;
    if (enable) {
        enableMapping(mapping);
    } else {
        disableMapping(mapping);
    }
}

float UserInputMapper::getValue(const Endpoint::Pointer& endpoint) const {
    Locker locker(_lock);
    auto valuesIterator = _overrides.find(endpoint);
    if (_overrides.end() != valuesIterator) {
        return valuesIterator->second->value();
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
    Locker locker(_lock);
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

static const QString JSON_NAME = QStringLiteral("name");
static const QString JSON_CHANNELS = QStringLiteral("channels");
static const QString JSON_CHANNEL_FROM = QStringLiteral("from");
static const QString JSON_CHANNEL_WHEN = QStringLiteral("when");
static const QString JSON_CHANNEL_TO = QStringLiteral("to");
static const QString JSON_CHANNEL_FILTERS = QStringLiteral("filters");

Endpoint::Pointer UserInputMapper::parseEndpoint(const QJsonValue& value) {
    Endpoint::Pointer result;
    if (value.isString()) {
        auto input = findDeviceInput(value.toString());
        result = endpointFor(input);
    } else if (value.isObject()) {
        // Endpoint is defined as an object, we expect a js function then
        return Endpoint::Pointer();
    }

    if (!result) {
        qWarning() << "Invalid endpoint definition " << value;
    }
    return result;
}

class AndConditional : public Conditional {
public:
    using Pointer = std::shared_ptr<AndConditional>;

    AndConditional(Conditional::List children) : _children(children) { }

    virtual bool satisfied() override {
        for (auto& conditional : _children) {
            if (!conditional->satisfied()) {
                return false;
            }
        }
        return true;
    }

private:
    Conditional::List _children;
};

class EndpointConditional : public Conditional {
public:
    EndpointConditional(Endpoint::Pointer endpoint) : _endpoint(endpoint) {}
    virtual bool satisfied() override { return _endpoint && _endpoint->value() != 0.0; }
private:
    Endpoint::Pointer _endpoint;
};

Conditional::Pointer UserInputMapper::parseConditional(const QJsonValue& value) {
    if (value.isArray()) {
        // Support "when" : [ "GamePad.RB", "GamePad.LB" ]
        Conditional::List children;
        for (auto arrayItem : value.toArray()) {
            Conditional::Pointer childConditional = parseConditional(arrayItem);
            if (!childConditional) {
                return Conditional::Pointer();
            }
            children.push_back(childConditional);
        }
        return std::make_shared<AndConditional>(children);
    } else if (value.isString()) {
        // Support "when" : "GamePad.RB"
        auto input = findDeviceInput(value.toString());
        auto endpoint = endpointFor(input);
        if (!endpoint) {
            return Conditional::Pointer();
        }

        return std::make_shared<EndpointConditional>(endpoint);
    }

    return Conditional::parse(value);
}


Filter::Pointer UserInputMapper::parseFilter(const QJsonValue& value) {
    Filter::Pointer result;
    if (value.isString()) {
        result = Filter::getFactory().create(value.toString());
    } else if (value.isObject()) {
        result = Filter::parse(value.toObject());
    } 

    if (!result) {
        qWarning() << "Invalid filter definition " << value;
    }
      
    return result;
}


Filter::List UserInputMapper::parseFilters(const QJsonValue& value) {
    if (value.isNull()) {
        return Filter::List();
    }

    if (value.isArray()) {
        Filter::List result;
        auto filtersArray = value.toArray();
        for (auto filterValue : filtersArray) {
            Filter::Pointer filter = parseFilter(filterValue);
            if (!filter) {
                return Filter::List();
            }
            result.push_back(filter);
        }
        return result;
    } 

    Filter::Pointer filter = parseFilter(value);
    if (!filter) {
        return Filter::List();
    }
    return Filter::List({ filter });
}

Endpoint::Pointer UserInputMapper::parseDestination(const QJsonValue& value) {
    if (value.isArray()) {
        ArrayEndpoint::Pointer result = std::make_shared<ArrayEndpoint>();
        for (auto arrayItem : value.toArray()) {
            Endpoint::Pointer destination = parseEndpoint(arrayItem);
            if (!destination) {
                return Endpoint::Pointer();
            }
            result->_children.push_back(destination);
        }
        return result;
    } 
    
    return parseEndpoint(value);
}

Endpoint::Pointer UserInputMapper::parseSource(const QJsonValue& value) {
    if (value.isArray()) {
        AnyEndpoint::Pointer result = std::make_shared<AnyEndpoint>();
        for (auto arrayItem : value.toArray()) {
            Endpoint::Pointer destination = parseEndpoint(arrayItem);
            if (!destination) {
                return Endpoint::Pointer();
            }
            result->_children.push_back(destination);
        }
        return result;
    }

    return parseEndpoint(value);
}

Route::Pointer UserInputMapper::parseRoute(const QJsonValue& value) {
    if (!value.isObject()) {
        return Route::Pointer();
    }

    const auto& obj = value.toObject();
    Route::Pointer result = std::make_shared<Route>();
    result->source = parseSource(obj[JSON_CHANNEL_FROM]);
    if (!result->source) {
        qWarning() << "Invalid route source " << obj[JSON_CHANNEL_FROM];
        return Route::Pointer();
    }


    result->destination = parseDestination(obj[JSON_CHANNEL_TO]);
    if (!result->destination) {
        qWarning() << "Invalid route destination " << obj[JSON_CHANNEL_TO];
        return Route::Pointer();
    }

    if (obj.contains(JSON_CHANNEL_WHEN)) {
        auto conditionalsValue = obj[JSON_CHANNEL_WHEN];
        result->conditional = parseConditional(conditionalsValue);
        if (!result->conditional) {
            qWarning() << "Invalid route conditionals " << conditionalsValue;
            return Route::Pointer();
        }
    }

    if (obj.contains(JSON_CHANNEL_FILTERS)) {
        auto filtersValue = obj[JSON_CHANNEL_FILTERS];
        result->filters = parseFilters(filtersValue);
        if (result->filters.empty()) {
            qWarning() << "Invalid route filters " << filtersValue;
            return Route::Pointer();
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


void UserInputMapper::enableMapping(const Mapping::Pointer& mapping) {
    Locker locker(_lock);
    // New routes for a device get injected IN FRONT of existing routes.  Routes
    // are processed in order so this ensures that the standard -> action processing 
    // takes place after all of the hardware -> standard or hardware -> action processing
    // because standard -> action is the first set of routes added.
    for (auto route : mapping->routes) {
        if (route->source->getInput().device == STANDARD_DEVICE) {
            _standardRoutes.push_front(route);
        } else {
            _deviceRoutes.push_front(route);
        }
    }
}

void UserInputMapper::disableMapping(const Mapping::Pointer& mapping) {
    Locker locker(_lock);
    const auto& deviceRoutes = mapping->routes;
    std::set<Route::Pointer> routeSet(deviceRoutes.begin(), deviceRoutes.end());

    // FIXME this seems to result in empty route pointers... need to find a better way to remove them.
    std::remove_if(_deviceRoutes.begin(), _deviceRoutes.end(), [&](Route::Pointer route)->bool {
        return routeSet.count(route) != 0;
    });
    std::remove_if(_standardRoutes.begin(), _standardRoutes.end(), [&](Route::Pointer route)->bool {
        return routeSet.count(route) != 0;
    });
}

}

#include "UserInputMapper.moc"
