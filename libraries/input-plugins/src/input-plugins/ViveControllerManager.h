//
//  ViveControllerManager.h
//  interface/src/devices
//
//  Created by Sam Gondelman on 6/29/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi__ViveControllerManager
#define hifi__ViveControllerManager

#include <QObject>
#include <unordered_set>

#include <GLMHelpers.h>

#include <model/Geometry.h>
#include <gpu/Texture.h>
#include "plugins/Plugin.h"
#include <RenderArgs.h>
#include "UserInputMapper.h"

class ViveControllerManager : public Plugin {
public:
    virtual const QString& getName() const override;
    virtual bool isSupported() const override;

    /// Called when plugin is initially loaded, typically at application start
    virtual void init() override;
    /// Called when application is shutting down
    virtual void deinit() override ;

    /// Called when a plugin is being activated for use.  May be called multiple times.
    virtual void activate(PluginContainer * container) override;
    /// Called when a plugin is no longer being used.  May be called multiple times.
    virtual void deactivate() override;

    /**
     * Called by the application during it's idle phase.  If the plugin needs to do
     * CPU intensive work, it should launch a thread for that, rather than trying to
     * do long operations in the idle call
     */
    virtual void idle() override;

    enum JoystickAxisChannel {
        AXIS_Y_POS = 1U << 1,
        AXIS_Y_NEG = 1U << 2,
        AXIS_X_POS = 1U << 3,
        AXIS_X_NEG = 1U << 4,
        BACK_TRIGGER = 1U << 5,
    };

    enum Axis {
        TRACKPAD_AXIS = 0,
        TRIGGER_AXIS,
        AXIS_2,
        AXIS_3,
        AXIS_4,
    };
    
    enum JointChannel {
        LEFT_HAND = 0,
        RIGHT_HAND,
    };
    
    void focusOutEvent();

    void render(RenderArgs* args);
    void update();
    
    static ViveControllerManager& getInstance();
    
    typedef std::unordered_set<int> ButtonPressedMap;
    typedef std::map<int, float> AxisStateMap;
    typedef std::map<int, UserInputMapper::PoseValue> PoseStateMap;
    
    float getButton(int channel) const;
    float getAxis(int channel) const;
    UserInputMapper::PoseValue getPose(int channel) const;
    
    UserInputMapper::Input makeInput(unsigned int button, int index);
    UserInputMapper::Input makeInput(JoystickAxisChannel axis, int index);
    UserInputMapper::Input makeInput(JointChannel joint);
    
    void registerToUserInputMapper(UserInputMapper& mapper);
    void assignDefaultInputMapping(UserInputMapper& mapper);
    
    void activate();
    
private:
    ViveControllerManager();
    ~ViveControllerManager();

    void renderHand(UserInputMapper::PoseValue pose, gpu::Batch& batch, int index);
    
    void handleButtonEvent(uint64_t buttons, int index);
    void handleAxisEvent(Axis axis, float x, float y, int index);
    void handlePoseEvent(const mat4& mat, int index);
    
    bool _isInitialized;
    bool _isEnabled;
    int _trackedControllers;

    bool _modelLoaded;
    model::Geometry _modelGeometry;
    gpu::TexturePointer texture;

    static const QString NAME;

protected:
    int _deviceID = 0;
    
    ButtonPressedMap _buttonPressedMap;
    AxisStateMap _axisStateMap;
    PoseStateMap _poseStateMap;
};


#endif // hifi__ViveControllerManager
