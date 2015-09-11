//
//  AnimationHandle.h
//  libraries/animation/src/
//
//  Created by Andrzej Kapolka on 10/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AnimationHandle_h
#define hifi_AnimationHandle_h

#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVector>

#include "AnimationCache.h"
#include "AnimationLoop.h"
#include "Rig.h"

class AnimationHandle;
class Model;

typedef std::shared_ptr<AnimationHandle> AnimationHandlePointer;
typedef std::weak_ptr<AnimationHandle> WeakAnimationHandlePointer;
inline uint qHash(const std::shared_ptr<AnimationHandle>& a, uint seed) {
    // return qHash(a.get(), seed);
    AnimationHandle* strongRef = a ? a.get() : nullptr;
    return qHash(strongRef, seed);
}
inline uint qHash(const std::weak_ptr<AnimationHandle>& a, uint seed) {
    AnimationHandlePointer strongPointer = a.lock();
    AnimationHandle* strongRef = strongPointer ? strongPointer.get() : nullptr;
    return qHash(strongRef, seed);
}


// inline uint qHash(const WeakAnimationHandlePointer& handle, uint seed) {
//     return qHash(handle.data(), seed);
// }



/// Represents a handle to a model animation. I.e., an Animation in use by a given Rig.
class AnimationHandle : public QObject, public std::enable_shared_from_this<AnimationHandle> {
    Q_OBJECT

public:

    AnimationHandle(RigPointer rig);

    AnimationHandlePointer getAnimationHandlePointer() { return shared_from_this(); }

    void setRole(const QString& role) { _role = role; }
    const QString& getRole() const { return _role; }

    void setURL(const QUrl& url);
    const QUrl& getURL() const { return _url; }

    void setPriority(float priority);
    float getPriority() const { return _priority; }
    void setMix(float mix) { _mix = mix; }
    void setFade(float fade) { _fade = fade; }
    float getFade() const { return _fade; }
    void setFadePerSecond(float fadePerSecond) { _fadePerSecond = fadePerSecond; }
    float getFadePerSecond() const { return _fadePerSecond; }

    void setMaskedJoints(const QStringList& maskedJoints);
    const QStringList& getMaskedJoints() const { return _maskedJoints; }


    void setFPS(float fps) { _animationLoop.setFPS(fps); }
    float getFPS() const { return _animationLoop.getFPS(); }

    void setLoop(bool loop) { _animationLoop.setLoop(loop); }
    bool getLoop() const { return _animationLoop.getLoop(); }

    void setHold(bool hold) { _animationLoop.setHold(hold); }
    bool getHold() const { return _animationLoop.getHold(); }

    void setStartAutomatically(bool startAutomatically);
    bool getStartAutomatically() const { return _animationLoop.getStartAutomatically(); }

    void setFirstFrame(float firstFrame) { _animationLoop.setFirstFrame(firstFrame); }
    float getFirstFrame() const { return _animationLoop.getFirstFrame(); }

    void setLastFrame(float lastFrame) { _animationLoop.setLastFrame(lastFrame); }
    float getLastFrame() const { return _animationLoop.getLastFrame(); }

    void setRunning(bool running, bool restoreJoints = true);
    bool isRunning() const { return _animationLoop.isRunning(); }

    void setFrameIndex(float frameIndex) { _animationLoop.setFrameIndex(frameIndex); }
    float getFrameIndex() const { return _animationLoop.getFrameIndex(); }

    AnimationDetails getAnimationDetails() const;
    void setAnimationDetails(const AnimationDetails& details);

    void setJointMappings(QVector<int> jointMappings);
    QVector<int> getJointMappings(); // computing if necessary
    void simulate(float deltaTime);
    void applyFrame(float frameIndex);
    void replaceMatchingPriorities(float newPriority);
    void restoreJoints();
    void clearJoints() { _jointMappings.clear(); }

signals:

    void runningChanged(bool running);

public slots:

    void start() { setRunning(true); }
    void stop() { setRunning(false); _fadePerSecond = _fade = 0.0f; }

private:

    RigPointer _rig;
    AnimationPointer _animation;
    QString _role;
    QUrl _url;
    float _priority;
    float _mix; // How much of this animation to blend against what is already there. 1.0 sets to just this animation.
    float _fade; // How far are we into full strength. 0.0 uses none of this animation, 1.0 (the max) is as much as possible.
    float _fadePerSecond; // How fast should _fade change? +1.0 means _fade is increasing to 1.0 in 1 second. Negative is fading out.

    QStringList _maskedJoints;
    QVector<int> _jointMappings;

    AnimationLoop _animationLoop;
};


#endif // hifi_AnimationHandle_h
