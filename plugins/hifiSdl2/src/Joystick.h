//
//  Joystick.h
//  input-plugins/src/input-plugins
//
//  Created by Stephen Birarda on 2014-09-23.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Joystick_h
#define hifi_Joystick_h

#include <qobject.h>
#include <qvector.h>

#include <SDL.h>
#undef main

#include <controllers/InputDevice.h>
#include <controllers/StandardControls.h>

class Joystick : public QObject, public controller::InputDevice {
    Q_OBJECT
    Q_PROPERTY(QString name READ getName)
    Q_PROPERTY(int instanceId READ getInstanceId)
    
public:
    using Pointer = std::shared_ptr<Joystick>;

    const QString getName() const { return _name; }

    SDL_GameController* getGameController() { return _sdlGameController; }

    // Device functions
    virtual controller::Input::NamedVector getAvailableInputs() const override;
    virtual QString getDefaultMappingConfig() const override;
    virtual void update(float deltaTime, const controller::InputCalibrationData& inputCalibrationData) override;
    virtual void focusOutEvent() override;

    bool triggerHapticPulse(float strength, float duration, controller::Hand hand) override;
    
    Joystick() : InputDevice("GamePad") {}
    ~Joystick();
    
    Joystick(SDL_JoystickID instanceId, SDL_GameController* sdlGameController);
    
    void closeJoystick();

    void handleAxisEvent(const SDL_ControllerAxisEvent& event);
    void handleButtonEvent(const SDL_ControllerButtonEvent& event);
    
    int getInstanceId() const { return _instanceId; }
    
private:
    SDL_GameController* _sdlGameController;
    SDL_Joystick* _sdlJoystick;
    SDL_Haptic* _sdlHaptic;
    SDL_JoystickID _instanceId;

    mutable controller::Input::NamedVector _availableInputs;
};

#endif // hifi_Joystick_h
