//
//  AnimationHandle.h
//  interface/src/renderer
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

#include <AnimationCache.h>
#include <AnimationLoop.h>

class AnimationHandle;
class Model;

typedef QSharedPointer<AnimationHandle> AnimationHandlePointer;
typedef QWeakPointer<AnimationHandle> WeakAnimationHandlePointer;


/// Represents a handle to a model animation.
class AnimationHandle : public QObject {
    Q_OBJECT
    
public:

    void setRole(const QString& role) { _role = role; }
    const QString& getRole() const { return _role; }

    void setURL(const QUrl& url);
    const QUrl& getURL() const { return _url; }

    void setPriority(float priority);
    float getPriority() const { return _priority; }

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
    
    void setRunning(bool running);
    bool isRunning() const { return _animationLoop.isRunning(); }

    void setFrameIndex(float frameIndex) { _animationLoop.setFrameIndex(frameIndex); }
    float getFrameIndex() const { return _animationLoop.getFrameIndex(); }

    AnimationDetails getAnimationDetails() const;
    void setAnimationDetails(const AnimationDetails& details);

signals:
    
    void runningChanged(bool running);

public slots:

    void start() { setRunning(true); }
    void stop() { setRunning(false); }
    
private:

    friend class Model;

    AnimationHandle(Model* model);
        
    void simulate(float deltaTime);
    void applyFrame(float frameIndex);
    void replaceMatchingPriorities(float newPriority);
    void restoreJoints();
    
    void clearJoints() { _jointMappings.clear(); }
    
    Model* _model;
    WeakAnimationHandlePointer _self;
    AnimationPointer _animation;
    QString _role;
    QUrl _url;
    float _priority;

    QStringList _maskedJoints;
    QVector<int> _jointMappings;
    
    AnimationLoop _animationLoop;
};


#endif // hifi_AnimationHandle_h
