//
//  PointerEvent.h
//  script-engine/src
//
//  Created by Anthony Thibault on 2016-8-11.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PointerEvent_h
#define hifi_PointerEvent_h

#include <stdint.h>
#include <glm/glm.hpp>
#include <QScriptValue>

class PointerEvent {
public:
    enum Button {
        NoButtons = 0x0,
        PrimaryButton = 0x1,
        SecondaryButton = 0x2,
        TertiaryButton = 0x4
    };

    enum EventType {
        Press,     // A button has just been pressed
        Release,   // A button has just been released
        Move       // The pointer has just moved
    };

    PointerEvent();
    PointerEvent(EventType type, uint32_t id,
                 const glm::vec2& pos2D, const glm::vec3& pos3D,
                 const glm::vec3& normal, const glm::vec3& direction,
                 Button button, uint32_t buttons);

    static QScriptValue toScriptValue(QScriptEngine* engine, const PointerEvent& event);
    static void fromScriptValue(const QScriptValue& object, PointerEvent& event);

    QScriptValue toScriptValue(QScriptEngine* engine) const { return PointerEvent::toScriptValue(engine, *this); }

    EventType getType() const { return _type; }
    uint32_t getID() const { return _id; }
    const glm::vec2& getPos2D() const { return _pos2D; }
    const glm::vec3& getPos3D() const { return _pos3D; }
    const glm::vec3& getNormal() const { return _normal; }
    const glm::vec3& getDirection() const { return _direction; }
    Button getButton() const { return _button; }
    uint32_t getButtons() const { return _buttons; }

private:
    EventType _type;
    uint32_t _id;         // used to identify the pointer.  (left vs right hand, for example)
    glm::vec2 _pos2D;     // (in meters) projected onto the xy plane of entities dimension box, (0, 0) is upper right hand corner
    glm::vec3 _pos3D;     // surface location in world coordinates (in meters)
    glm::vec3 _normal;    // surface normal
    glm::vec3 _direction; // incoming direction of pointer ray.

    Button _button { NoButtons };  // button assosiated with this event, (if type is Press, this will be the button that is pressed)
    uint32_t _buttons { NoButtons }; // the current state of all the buttons.
};

Q_DECLARE_METATYPE(PointerEvent)

#endif // hifi_PointerEvent_h
