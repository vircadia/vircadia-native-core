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
    
    void setFPS(float fps) { _fps = fps; }
    float getFPS() const { return _fps; }

    void setPriority(float priority);
    float getPriority() const { return _priority; }
    
    void setLoop(bool loop) { _loop = loop; }
    bool getLoop() const { return _loop; }
    
    void setHold(bool hold) { _hold = hold; }
    bool getHold() const { return _hold; }
    
    void setStartAutomatically(bool startAutomatically);
    bool getStartAutomatically() const { return _startAutomatically; }
    
    void setFirstFrame(float firstFrame) { _firstFrame = firstFrame; }
    float getFirstFrame() const { return _firstFrame; }
    
    void setLastFrame(float lastFrame) { _lastFrame = lastFrame; }
    float getLastFrame() const { return _lastFrame; }
    
    void setMaskedJoints(const QStringList& maskedJoints);
    const QStringList& getMaskedJoints() const { return _maskedJoints; }
    
    void setRunning(bool running);
    bool isRunning() const { return _running; }

    void setFrameIndex(float frameIndex) { _frameIndex = glm::clamp(_frameIndex, _firstFrame, _lastFrame); }
    float getFrameIndex() const { return _frameIndex; }

    AnimationDetails getAnimationDetails() const;

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
    
    Model* _model;
    WeakAnimationHandlePointer _self;
    AnimationPointer _animation;
    QString _role;
    QUrl _url;
    float _fps;
    float _priority;
    bool _loop;
    bool _hold;
    bool _startAutomatically;
    float _firstFrame;
    float _lastFrame;
    QStringList _maskedJoints;
    bool _running;
    QVector<int> _jointMappings;
    float _frameIndex;
};


#endif // hifi_AnimationHandle_h
