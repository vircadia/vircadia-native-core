//
//  ScriptableAvatar.cpp
//
//
//  Created by Clement on 7/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QThread>

#include <GLMHelpers.h>

#include "ScriptableAvatar.h"

ScriptableAvatar::ScriptableAvatar(ScriptEngine* scriptEngine) : _scriptEngine(scriptEngine), _animation(NULL) {
    connect(_scriptEngine, SIGNAL(update(float)), this, SLOT(update(float)));
}

// hold and priority unused but kept so that client side JS can run.
void ScriptableAvatar::startAnimation(const QString& url, float fps, float priority,
                              bool loop, bool hold, float firstFrame, float lastFrame, const QStringList& maskedJoints) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "startAnimation", Q_ARG(const QString&, url), Q_ARG(float, fps),
                                  Q_ARG(float, priority), Q_ARG(bool, loop), Q_ARG(bool, hold), Q_ARG(float, firstFrame),
                                  Q_ARG(float, lastFrame), Q_ARG(const QStringList&, maskedJoints));
        return;
    }
    _animation = DependencyManager::get<AnimationCache>()->getAnimation(url);
    _animationDetails = AnimationDetails("", QUrl(url), fps, 0, loop, hold, false, firstFrame, lastFrame, true, firstFrame);
    _maskedJoints = maskedJoints;
}

void ScriptableAvatar::stopAnimation() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "stopAnimation");
        return;
    }
    _animation.clear();
}

AnimationDetails ScriptableAvatar::getAnimationDetails() {
    if (QThread::currentThread() != thread()) {
        AnimationDetails result;
        QMetaObject::invokeMethod(this, "getAnimationDetails", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(AnimationDetails, result));
        return result;
    }
    return _animationDetails;
}

void ScriptableAvatar::update(float deltatime) {
    // Run animation
    if (_animation && _animation->isLoaded() && _animation->getFrames().size() > 0) {
        QStringList modelJoints = getJointNames();
        QStringList animationJoints = _animation->getJointNames();
        
        if (_jointData.size() != modelJoints.size()) {
            _jointData.resize(modelJoints.size());
        }
        
        float currentFrame = _animationDetails.currentFrame + deltatime * _animationDetails.fps;
        if (_animationDetails.loop || currentFrame < _animationDetails.lastFrame) {
            while (currentFrame >= _animationDetails.lastFrame) {
                currentFrame -= (_animationDetails.lastFrame - _animationDetails.firstFrame);
            }
            _animationDetails.currentFrame = currentFrame;
            
            const int frameCount = _animation->getFrames().size();
            const FBXAnimationFrame& floorFrame = _animation->getFrames().at((int)glm::floor(currentFrame) % frameCount);
            const FBXAnimationFrame& ceilFrame = _animation->getFrames().at((int)glm::ceil(currentFrame) % frameCount);
            const float frameFraction = glm::fract(currentFrame);
            
            for (int i = 0; i < modelJoints.size(); i++) {
                int mapping = animationJoints.indexOf(modelJoints[i]);
                if (mapping != -1 && !_maskedJoints.contains(modelJoints[i])) {
                    JointData& data = _jointData[i];

                    auto newRotation = safeMix(floorFrame.rotations.at(i), ceilFrame.rotations.at(i), frameFraction);
                    auto newTranslation = floorFrame.translations.at(i) * (1.0f - frameFraction) +
                        ceilFrame.translations.at(i) * frameFraction;

                    if (data.rotation != newRotation) {
                        data.rotation = newRotation;
                        data.rotationSet = true;
                    }
                    if (data.translation != newTranslation) {
                        data.translation = newTranslation;
                        data.translationSet = true;
                    }
                }
            }
        } else {
            _animation.clear();
        }
    }
}
