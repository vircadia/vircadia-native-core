//
//  UserInputMapper.h
//  input-plugins/src/input-plugins
//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UserInputMapper_h
#define hifi_UserInputMapper_h

#include <glm/glm.hpp>

#include <unordered_set>
#include <functional>
#include <memory>
#include <DependencyManager.h>
#include <RegisteredMetaTypes.h>

class StandardController;    
typedef std::shared_ptr<StandardController> StandardControllerPointer;

class UserInputMapper : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY
    Q_ENUMS(Action)
public:
    ~UserInputMapper();

    typedef unsigned short uint16;
    typedef unsigned int uint32;

    enum class ChannelType {
        UNKNOWN = 0,
        BUTTON = 1,
        AXIS,
        POSE,
    };

    // Input is the unique identifier to find a n input channel of a particular device
    // Devices are responsible for registering to the UseInputMapper so their input channels can be sued and mapped
    // to the Action channels
    class Input {
    public:
        union {
            struct {
                uint16 _device; // Up to 64K possible devices
                uint16 _channel : 14; // 2^14 possible channel per Device
                uint16 _type : 2; // 2 bits to store the Type directly in the ID
            };
            uint32 _id = 0; // by default Input is 0 meaning invalid
        };
        
        bool isValid() const { return (_id != 0); }
        
        uint16 getDevice() const { return _device; }
        uint16 getChannel() const { return _channel; }
        uint32 getID() const { return _id; }
        ChannelType getType() const { return (ChannelType) _type; }
        
        void setDevice(uint16 device) { _device = device; }
        void setChannel(uint16 channel) { _channel = channel; }
        void setType(uint16 type) { _type = type; }
        void setID(uint32 ID) { _id = ID; }

        bool isButton() const { return getType() == ChannelType::BUTTON; }
        bool isAxis() const { return getType() == ChannelType::AXIS; }
        bool isPose() const { return getType() == ChannelType::POSE; }

        // WORKAROUND: the explicit initializer here avoids a bug in GCC-4.8.2 (but not found in 4.9.2)
        // where the default initializer (a C++-11ism) for the union data above is not applied.
        explicit Input() : _id(0) {}
        explicit Input(uint32 id) : _id(id) {}
        explicit Input(uint16 device, uint16 channel, ChannelType type) : _device(device), _channel(channel), _type(uint16(type)) {}
        Input(const Input& src) : _id(src._id) {}
        Input& operator = (const Input& src) { _id = src._id; return (*this); }
        bool operator ==(const Input& right) const { return _id == right._id; }
        bool operator < (const Input& src) const { return _id < src._id; }
    };

    // Modifiers are just button inputID
    typedef std::vector< Input > Modifiers;

    class PoseValue {
    public:
        glm::vec3 _translation{ 0.0f };
        glm::quat _rotation;
        bool _valid;

        PoseValue() : _valid(false) {};
        PoseValue(glm::vec3 translation, glm::quat rotation) : _translation(translation), _rotation(rotation), _valid(true) {}
        PoseValue(const PoseValue&) = default;
        PoseValue& operator = (const PoseValue&) = default;
        bool operator ==(const PoseValue& right) const { return _translation == right.getTranslation() && _rotation == right.getRotation() && _valid == right.isValid(); }
        
        bool isValid() const { return _valid; }
        glm::vec3 getTranslation() const { return _translation; }
        glm::quat getRotation() const { return _rotation; }
    };
    
    typedef std::function<bool (const Input& input, int timestamp)> ButtonGetter;
    typedef std::function<float (const Input& input, int timestamp)> AxisGetter;
    typedef std::function<PoseValue (const Input& input, int timestamp)> PoseGetter;
    typedef QPair<Input, QString> InputPair;
    typedef std::function<QVector<InputPair> ()> AvailableInputGetter;
    typedef std::function<bool ()> ResetBindings;
    
    typedef QVector<InputPair> AvailableInput;

   class DeviceProxy {
    public:
       DeviceProxy(QString name) { _name = name; }
       const QString& getName() const { return _name; }

       QString _name;
       ButtonGetter getButton = [] (const Input& input, int timestamp) -> bool { return false; };
       AxisGetter getAxis = [] (const Input& input, int timestamp) -> float { return 0.0f; };
       PoseGetter getPose = [] (const Input& input, int timestamp) -> PoseValue { return PoseValue(); };
       AvailableInputGetter getAvailabeInputs = [] () -> AvailableInput { return QVector<InputPair>(); };
       ResetBindings resetDeviceBindings = [] () -> bool { return true; };
       
       typedef std::shared_ptr<DeviceProxy> Pointer;
    };
    // GetFreeDeviceID should be called before registering a device to use an ID not used by a different device.
    uint16 getFreeDeviceID() { return _nextFreeDeviceID++; }
    bool registerDevice(uint16 deviceID, const DeviceProxy::Pointer& device);
    bool registerStandardDevice(const DeviceProxy::Pointer& device) { _standardDevice = device; return true; }
    DeviceProxy::Pointer getDeviceProxy(const Input& input);
    QString getDeviceName(uint16 deviceID);
    QVector<InputPair> getAvailableInputs(uint16 deviceID) { return _registeredDevices[deviceID]->getAvailabeInputs(); }
    void resetAllDeviceBindings();
    void resetDevice(uint16 deviceID);
    int findDevice(QString name);
    QVector<QString> getDeviceNames();

    // Actions are the output channels of the Mapper, that's what the InputChannel map to
    // For now the Actions are hardcoded, this is bad, but we will fix that in the near future
    enum Action {
        LONGITUDINAL_BACKWARD = 0,
        LONGITUDINAL_FORWARD,

        LATERAL_LEFT,
        LATERAL_RIGHT,

        VERTICAL_DOWN,
        VERTICAL_UP,

        YAW_LEFT,
        YAW_RIGHT,
 
        PITCH_DOWN,
        PITCH_UP,
 
        BOOM_IN,
        BOOM_OUT,
        
        LEFT_HAND,
        RIGHT_HAND,

        LEFT_HAND_CLICK,
        RIGHT_HAND_CLICK,

        SHIFT,
        
        ACTION1,
        ACTION2,

        CONTEXT_MENU,
        TOGGLE_MUTE,

        NUM_ACTIONS,
    };
    
    std::vector<QString> _actionNames = std::vector<QString>(NUM_ACTIONS);
    void createActionNames();

    QVector<Action> getAllActions() const;
    QString getActionName(Action action) const { return UserInputMapper::_actionNames[(int) action]; }
    float getActionState(Action action) const { return _actionStates[action]; }
    PoseValue getPoseState(Action action) const { return _poseStates[action]; }
    int findAction(const QString& actionName) const;
    QVector<QString> getActionNames() const;
    void assignDefaulActionScales();

    // Add input channel to the mapper and check that all the used channels are registered.
    // Return true if theinput channel is created correctly, false either
    bool addInputChannel(Action action, const Input& input, float scale = 1.0f);
    bool addInputChannel(Action action, const Input& input, const Input& modifer, float scale = 1.0f);
    
    // Under the hood, the input channels are organized in map sorted on the _output
    // The InputChannel class is just the full values describing the input channel in one object 
    class InputChannel {
    public:
        Input _input;
        Input _modifier = Input(); // make it invalid by default, meaning no modifier
        Action _action = LONGITUDINAL_BACKWARD;
        float _scale = 0.0f;
        
        Input getInput() const { return _input; }
        Input getModifier() const { return _modifier; }
        Action getAction() const { return _action; }
        float getScale() const { return _scale; }
        
        void setInput(Input input) { _input = input; }
        void setModifier(Input modifier) { _modifier = modifier; }
        void setAction(Action action) { _action = action; }
        void setScale(float scale) { _scale = scale; }

        InputChannel() {}
        InputChannel(const Input& input, const Input& modifier, Action action, float scale = 1.0f) :
            _input(input), _modifier(modifier), _action(action), _scale(scale) {}
        InputChannel(const InputChannel& src) : InputChannel(src._input, src._modifier, src._action, src._scale) {}
        InputChannel& operator = (const InputChannel& src) { _input = src._input; _modifier = src._modifier; _action = src._action; _scale = src._scale; return (*this); }
        bool operator ==(const InputChannel& right) const { return _input == right._input && _modifier == right._modifier && _action == right._action && _scale == right._scale; }
        bool hasModifier() { return _modifier.isValid(); }
    };
    typedef std::vector< InputChannel > InputChannels;

    // Add a bunch of input channels, return the true number of channels that successfully were added
    int addInputChannels(const InputChannels& channels);
    // Remove the first found instance of the input channel from the input mapper, true if found
    bool removeInputChannel(InputChannel channel);
    void removeAllInputChannels();
    void removeAllInputChannelsForDevice(uint16 device);
    void removeDevice(int device);
    //Grab all the input channels currently in use, return the number
    int getInputChannels(InputChannels& channels) const;
    QVector<InputChannel> getAllInputsForDevice(uint16 device);
    QVector<InputChannel> getInputChannelsForAction(UserInputMapper::Action action);
    std::multimap<Action, InputChannel> getActionToInputsMap() { return _actionToInputsMap; }

    // Update means go grab all the device input channels and update the output channel values
    void update(float deltaTime);
    
    void setSensorToWorldMat(glm::mat4 sensorToWorldMat) { _sensorToWorldMat = sensorToWorldMat; }
    glm::mat4 getSensorToWorldMat() { return _sensorToWorldMat; }
    
    UserInputMapper();

    typedef std::map<int, DeviceProxy::Pointer> DevicesMap;
    DevicesMap getDevices() { return _registeredDevices; }

    uint16 getStandardDeviceID() const { return _standardDeviceID; }
    DeviceProxy::Pointer getStandardDevice() { return _standardDevice; }

signals:
    void actionEvent(int action, float state);


protected:
    void registerStandardDevice();
    uint16 _standardDeviceID = 0;
    DeviceProxy::Pointer _standardDevice;
    StandardControllerPointer _standardController;
        
    DevicesMap _registeredDevices;
    uint16 _nextFreeDeviceID = 1;

    typedef std::map<int, Modifiers> InputToMoModifiersMap;
    InputToMoModifiersMap _inputToModifiersMap;

    typedef std::multimap<Action, InputChannel> ActionToInputsMap;
    ActionToInputsMap _actionToInputsMap;
 
    std::vector<float> _actionStates = std::vector<float>(NUM_ACTIONS, 0.0f);
    std::vector<float> _actionScales = std::vector<float>(NUM_ACTIONS, 1.0f);
    std::vector<float> _lastActionStates = std::vector<float>(NUM_ACTIONS, 0.0f);
    std::vector<PoseValue> _poseStates = std::vector<PoseValue>(NUM_ACTIONS);

    glm::mat4 _sensorToWorldMat;
};

Q_DECLARE_METATYPE(UserInputMapper::InputPair)
Q_DECLARE_METATYPE(QVector<UserInputMapper::InputPair>)
Q_DECLARE_METATYPE(UserInputMapper::Input)
Q_DECLARE_METATYPE(UserInputMapper::InputChannel)
Q_DECLARE_METATYPE(QVector<UserInputMapper::InputChannel>)
Q_DECLARE_METATYPE(UserInputMapper::Action)
Q_DECLARE_METATYPE(QVector<UserInputMapper::Action>)

#endif // hifi_UserInputMapper_h
