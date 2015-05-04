//
//  UserInputMapper.h
//  interface/src/ui
//
//  Created by Sam Gateau on 4/27/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_UserInputMapper_h
#define hifi_UserInputMapper_h

#include <QTouchEvent>
#include <glm/glm.hpp>
#include <RegisteredMetaTypes.h>

#include <unordered_set>
#include <functional>
#include <memory>
#include <chrono>

class UserInputMapper : public QObject {
    Q_OBJECT
public:
    typedef unsigned short uint16;
    typedef unsigned int uint32;

    enum class ChannelType {
        UNKNOWN = 0,
        BUTTON = 1,
        AXIS,
        JOINT,
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
        bool isButton() const { return getType() == ChannelType::BUTTON; }
        bool isAxis() const { return getType() == ChannelType::AXIS; }
        bool isJoint() const { return getType() == ChannelType::JOINT; }

        explicit Input() {}
        explicit Input(uint32 id) : _id(id) {}
        explicit Input(uint16 device, uint16 channel, ChannelType type) : _device(device), _channel(channel), _type(uint16(type)) {}
        Input(const Input& src) : _id(src._id) {}
        Input& operator = (const Input& src) { _id = src._id; return (*this); }
        bool operator < (const Input& src) const { return _id < src._id; }
    };

    // Modifiers are just button inputID
    typedef std::vector< Input > Modifiers;

    class JointValue {
    public:
        glm::vec3 translation{ 0.0f };
        glm::quat rotation;

        JointValue() {};
        JointValue(const JointValue&) = default;
        JointValue& operator = (const JointValue&) = default;
    };
    
    typedef std::function<bool (const Input& input, int timestamp)> ButtonGetter;
    typedef std::function<float (const Input& input, int timestamp)> AxisGetter;
    typedef std::function<JointValue (const Input& input, int timestamp)> JointGetter;

    class DeviceProxy {
    public:
        DeviceProxy() {}
        
        ButtonGetter getButton = [] (const Input& input, int timestamp) -> bool { return false; };
        AxisGetter getAxis = [] (const Input& input, int timestamp) -> bool { return 0.0f; };
        JointGetter getJoint = [] (const Input& input, int timestamp) -> JointValue { return JointValue(); };
        
        typedef std::shared_ptr<DeviceProxy> Pointer;
    };
    bool registerDevice(uint16 deviceID, const DeviceProxy::Pointer& device);
    DeviceProxy::Pointer getDeviceProxy(const Input& input);


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

        NUM_ACTIONS,
    };

    float getActionState(Action action) const { return _actionStates[action]; }
    void assignDefaulActionUnitScales();

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

        InputChannel() {}
        InputChannel(const Input& input, const Input& modifier, Action action, float scale = 1.0f) :
            _input(input), _modifier(modifier), _action(action), _scale(scale) {}
        InputChannel(const InputChannel& src) : InputChannel(src._input, src._modifier, src._action, src._scale) {}
        InputChannel& operator = (const InputChannel& src) { _input = src._input; _modifier = src._modifier; _action = src._action; _scale = src._scale; return (*this); }
    
        bool hasModifier() { return _modifier.isValid(); }
    };
    typedef std::vector< InputChannel > InputChannels;

    // Add a bunch of input channels, return the true number of channels that successfully were added
    int addInputChannels(const InputChannels& channels);
    //Grab all the input channels currently in use, return the number
    int getInputChannels(InputChannels& channels) const;

    // Update means go grab all the device input channels and update the output channel values
    void update(float deltaTime);

    // Default contruct allocate the poutput size with the current hardcoded action channels
    UserInputMapper() { assignDefaulActionUnitScales(); }

protected:
    typedef std::map<int, DeviceProxy::Pointer> DevicesMap;
    DevicesMap _registeredDevices;
    
    typedef std::map<int, Modifiers> InputToMoModifiersMap;
    InputToMoModifiersMap _inputToModifiersMap;

    typedef std::multimap<Action, InputChannel> ActionToInputsMap;
    ActionToInputsMap _actionToInputsMap;
 
    std::vector<float> _actionStates = std::vector<float>(NUM_ACTIONS, 0.0f);
    std::vector<float> _actionUnitScales = std::vector<float>(NUM_ACTIONS, 1.0f);
};


class KeyboardMouseDevice {
public:

    enum KeyboardChannel {
        KEYBOARD_FIRST = 0,
        KEYBOARD_LAST = 255,
        KEYBOARD_MASK = 0x00FF,
    };

    enum MouseButtonChannel {
        MOUSE_BUTTON_LEFT = KEYBOARD_LAST + 1,
        MOUSE_BUTTON_RIGHT,
        MOUSE_BUTTON_MIDDLE,
    };

    enum MouseAxisChannel {
        MOUSE_AXIS_X_POS = MOUSE_BUTTON_MIDDLE + 1,
        MOUSE_AXIS_X_NEG,
        MOUSE_AXIS_Y_POS,
        MOUSE_AXIS_Y_NEG,
        MOUSE_AXIS_WHEEL_Y_POS,
        MOUSE_AXIS_WHEEL_Y_NEG,
        MOUSE_AXIS_WHEEL_X_POS,
        MOUSE_AXIS_WHEEL_X_NEG,
    };
        
    enum TouchAxisChannel {
        TOUCH_AXIS_X_POS = MOUSE_AXIS_WHEEL_X_NEG + 1,
        TOUCH_AXIS_X_NEG,
        TOUCH_AXIS_Y_POS,
        TOUCH_AXIS_Y_NEG,
    };
        
    enum TouchButtonChannel {
        TOUCH_BUTTON_PRESS = TOUCH_AXIS_Y_NEG + 1,
    };

    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap; // 8 axes

    void keyPressEvent(QKeyEvent* event);
    void keyReleaseEvent(QKeyEvent* event);

    void mouseMoveEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mousePressEvent(QMouseEvent* event, unsigned int deviceID = 0);
    void mouseReleaseEvent(QMouseEvent* event, unsigned int deviceID = 0);

    void touchBeginEvent(const QTouchEvent* event);
    void touchEndEvent(const QTouchEvent* event);
    void touchUpdateEvent(const QTouchEvent* event);

    void wheelEvent(QWheelEvent* event);

    ButtonPressedMap _buttonPressedMap;
    mutable AxisStateMap _axisStateMap;

    QPoint _lastCursor;
    glm::vec2 _lastTouch;
    bool _isTouching = false;

    float getButton(int channel) const;
    float getAxis(int channel) const;

    // Reserverd device ID for the standard keyboard and mouse
    const static UserInputMapper::uint16 DEFAULT_DESKTOP_DEVICE = 1;

    // Let's make it easy for Qt because we assume we love Qt forever
    static UserInputMapper::Input makeInput(Qt::Key code);
    static UserInputMapper::Input makeInput(Qt::MouseButton code);
    static UserInputMapper::Input makeInput(KeyboardMouseDevice::MouseAxisChannel axis);
    static UserInputMapper::Input makeInput(KeyboardMouseDevice::TouchAxisChannel axis);
    static UserInputMapper::Input makeInput(KeyboardMouseDevice::TouchButtonChannel button);
    
    KeyboardMouseDevice() {}

    void registerToUserInputMapper(UserInputMapper& mapper);
    void assignDefaultInputMapping(UserInputMapper& mapper);

    void resetDeltas() {
        _axisStateMap.clear();
    }

    glm::vec2 evalAverageTouchPoints(const QList<QTouchEvent::TouchPoint>& points) const;
    std::chrono::high_resolution_clock _clock;
    std::chrono::high_resolution_clock::time_point _lastTouchTime;
};

#endif // hifi_UserInputMapper_h