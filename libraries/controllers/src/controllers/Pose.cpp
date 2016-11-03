//
//  Created by Bradley Austin Davis on 2015/10/18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtScript/QScriptEngine>
#include <QtScript/QScriptValue>

#include <RegisteredMetaTypes.h>

#include "Pose.h"

namespace controller {

    Pose::Pose(const vec3& translation, const quat& rotation,
            const vec3& velocity, const vec3& angularVelocity) :
            translation(translation), rotation(rotation), velocity(velocity), angularVelocity(angularVelocity), valid (true) { }

    bool Pose::operator==(const Pose& right) const {
        // invalid poses return false for comparison, even against identical invalid poses, like NaN
        if (!valid || !right.valid) {
            return false;
        }

        // FIXME add margin of error?  Or add an additional withinEpsilon function?
        return translation == right.getTranslation() && rotation == right.getRotation() &&
            velocity == right.getVelocity() && angularVelocity == right.getAngularVelocity();
    }

    QScriptValue Pose::toScriptValue(QScriptEngine* engine, const Pose& pose) {
        QScriptValue obj = engine->newObject();
        obj.setProperty("translation", vec3toScriptValue(engine, pose.translation));
        obj.setProperty("rotation", quatToScriptValue(engine, pose.rotation));
        obj.setProperty("velocity", vec3toScriptValue(engine, pose.velocity));
        obj.setProperty("angularVelocity", vec3toScriptValue(engine, pose.angularVelocity));
        obj.setProperty("valid", pose.valid);

        return obj;
    }

    void Pose::fromScriptValue(const QScriptValue& object, Pose& pose) {
        auto translation = object.property("translation");
        auto rotation = object.property("rotation");
        auto velocity = object.property("velocity");
        auto angularVelocity = object.property("angularVelocity");
        if (translation.isValid() &&
                rotation.isValid() &&
                velocity.isValid() &&
                angularVelocity.isValid()) {
            vec3FromScriptValue(translation, pose.translation);
            quatFromScriptValue(rotation, pose.rotation);
            vec3FromScriptValue(velocity, pose.velocity);
            vec3FromScriptValue(angularVelocity, pose.angularVelocity);
            pose.valid = true;
        } else {
            pose.valid = false;
        }
    }

    Pose Pose::transform(const glm::mat4& mat) const {
        auto rot = glmExtractRotation(mat);
        Pose pose(transformPoint(mat, translation),
                  rot * rotation,
                  transformVectorFast(mat, velocity),
                  rot * angularVelocity);
        pose.valid = valid;
        return pose;
    }
}

