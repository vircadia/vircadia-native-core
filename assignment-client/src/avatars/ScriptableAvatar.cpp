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

void ScriptableAvatar::setSkeletonModelURL(const QUrl& skeletonModelURL) {
    _bind.reset();
    _animSkeleton.reset();
    AvatarData::setSkeletonModelURL(skeletonModelURL);
}
void ScriptableAvatar::update(float deltatime) {
    if (_bind.isNull() && !_skeletonFBXURL.isEmpty()) { // AvatarData will parse the .fst, but not get the .fbx skeleton.
        _bind = DependencyManager::get<AnimationCache>()->getAnimation(_skeletonFBXURL);
    }

    // Run animation
    if (_animation && _animation->isLoaded() && _animation->getFrames().size() > 0 && !_bind.isNull() && _bind->isLoaded()) {
        if (!_animSkeleton) {
            _animSkeleton = std::make_shared<AnimSkeleton>(_bind->getGeometry());
        }
        float currentFrame = _animationDetails.currentFrame + deltatime * _animationDetails.fps;
        if (_animationDetails.loop || currentFrame < _animationDetails.lastFrame) {
            while (currentFrame >= _animationDetails.lastFrame) {
                currentFrame -= (_animationDetails.lastFrame - _animationDetails.firstFrame);
            }
            _animationDetails.currentFrame = currentFrame;

            const QVector<FBXJoint>& modelJoints = _bind->getGeometry().joints;
            QStringList animationJointNames = _animation->getJointNames();

            const int nJoints = modelJoints.size();
            if (_jointData.size() != nJoints) {
                _jointData.resize(nJoints);
            }
            
            const int frameCount = _animation->getFrames().size();
            const FBXAnimationFrame& floorFrame = _animation->getFrames().at((int)glm::floor(currentFrame) % frameCount);
            const FBXAnimationFrame& ceilFrame = _animation->getFrames().at((int)glm::ceil(currentFrame) % frameCount);
            const float frameFraction = glm::fract(currentFrame);
            std::vector<AnimPose> poses = _animSkeleton->getRelativeDefaultPoses();
            
            for (int i = 0; i < animationJointNames.size(); i++) {
                const QString& name = animationJointNames[i];
                // As long as we need the model preRotations anyway, let's get the jointIndex from the bind skeleton rather than
                // trusting the .fst (which is sometimes not updated to match changes to .fbx).
                int mapping = _bind->getGeometry().getJointIndex(name);
                if (mapping != -1 && !_maskedJoints.contains(name)) {
                    // Eventually, this should probably deal with post rotations and translations, too.
                    poses[mapping].rot() = modelJoints[mapping].preRotation *
                        safeMix(floorFrame.rotations.at(i), ceilFrame.rotations.at(i), frameFraction);;
                 }
            }
            _animSkeleton->convertRelativePosesToAbsolute(poses);
            for (int i = 0; i < nJoints; i++) {
                JointData& data = _jointData[i];
                AnimPose& pose = poses[i];
                if (data.rotation != pose.rot()) {
                    data.rotation = pose.rot();
                    data.rotationSet = true;
                }
            }

        } else {
            _animation.clear();
        }
    }
}
