//
//  AnimationObject.h
//  libraries/animation/src/
//
//  Created by Andrzej Kapolka on 4/17/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimationObject_h
#define hifi_AnimationObject_h

#include <QObject>
#include <QScriptable>

#include <FBXReader.h>

class QScriptEngine;

/// Scriptable wrapper for animation pointers.
class AnimationObject : public QObject, protected QScriptable {
    Q_OBJECT
    Q_PROPERTY(QStringList jointNames READ getJointNames)
    Q_PROPERTY(QVector<HFMAnimationFrame> frames READ getFrames)

public:
    
    Q_INVOKABLE QStringList getJointNames() const;
    
    Q_INVOKABLE QVector<HFMAnimationFrame> getFrames() const;
};

/// Scriptable wrapper for animation frames.
class AnimationFrameObject : public QObject, protected QScriptable {
    Q_OBJECT
    Q_PROPERTY(QVector<glm::quat> rotations READ getRotations)

public:
    
    Q_INVOKABLE QVector<glm::quat> getRotations() const;
};

void registerAnimationTypes(QScriptEngine* engine);

#endif // hifi_AnimationObject_h
