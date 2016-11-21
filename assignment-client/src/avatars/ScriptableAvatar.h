//
//  ScriptableAvatar.h
//
//
//  Created by Clement on 7/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptableAvatar_h
#define hifi_ScriptableAvatar_h

#include <AnimationCache.h>
#include <AnimSkeleton.h>
#include <AvatarData.h>
#include <ScriptEngine.h>

class ScriptableAvatar : public AvatarData, public Dependency {
    Q_OBJECT
public:
   
    /// Allows scripts to run animations.
    Q_INVOKABLE void startAnimation(const QString& url, float fps = 30.0f, float priority = 1.0f, bool loop = false,
                                    bool hold = false, float firstFrame = 0.0f, float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList());
    Q_INVOKABLE void stopAnimation();
    Q_INVOKABLE AnimationDetails getAnimationDetails();
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL) override;
    
private slots:
    void update(float deltatime);
    
private:
    AnimationPointer _animation;
    AnimationDetails _animationDetails;
    QStringList _maskedJoints;
    AnimationPointer _bind; // a sleazy way to get the skeleton, given the various library/cmake dependencies
    std::shared_ptr<AnimSkeleton> _animSkeleton;
};

#endif // hifi_ScriptableAvatar_h
