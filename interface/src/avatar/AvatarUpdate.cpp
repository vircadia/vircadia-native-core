//
//  AvatarUpdate.cpp
//  interface/src/avatar
//
//  Created by Howard Stearns on 8/18/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

#include <DependencyManager.h>
#include "Application.h"
#include "AvatarManager.h"
#include "AvatarUpdate.h"
#include <display-plugins/DisplayPlugin.h>
#include "InterfaceLogging.h"

AvatarUpdate::AvatarUpdate() : GenericThread(),  _lastAvatarUpdate(0) {
    setObjectName("Avatar Update"); // GenericThread::initialize uses this to set the thread name.
    Settings settings;
    const int DEFAULT_TARGET_AVATAR_SIMRATE = 60;
    _targetInterval = USECS_PER_SECOND / settings.value("AvatarUpdateTargetSimrate", DEFAULT_TARGET_AVATAR_SIMRATE).toInt();
}
// We could have the constructor call initialize(), but GenericThread::initialize can take parameters.
// Keeping it separately called by the client allows that client to pass those without our
// constructor needing to know about them.

void AvatarUpdate::synchronousProcess() {

    // Keep our own updated value, so that our asynchronous code can consult it.
    _isHMDMode = qApp->isHMDMode();
    auto frameCount = qApp->getFrameCount();

    QSharedPointer<AvatarManager> manager = DependencyManager::get<AvatarManager>();
    MyAvatar* myAvatar = manager->getMyAvatar();
    assert(myAvatar);

    // transform the head pose from the displayPlugin into avatar coordinates.
    glm::mat4 invAvatarMat = glm::inverse(createMatFromQuatAndPos(myAvatar->getOrientation(), myAvatar->getPosition()));
    _headPose = invAvatarMat * (myAvatar->getSensorToWorldMatrix() * qApp->getActiveDisplayPlugin()->getHeadPose(frameCount));

    if (!isThreaded()) {
        process();
    }
}

bool AvatarUpdate::process() {
    PerformanceTimer perfTimer("AvatarUpdate");
    quint64 start = usecTimestampNow();
    quint64 deltaMicroseconds = 10000;
    if (_lastAvatarUpdate > 0) {
        deltaMicroseconds = start - _lastAvatarUpdate;
    } else {
        deltaMicroseconds = 10000; // 10 ms
    }
    float deltaSeconds = (float) deltaMicroseconds / (float) USECS_PER_SECOND;
    _lastAvatarUpdate = start;
    qApp->setAvatarSimrateSample(1.0f / deltaSeconds);

    QSharedPointer<AvatarManager> manager = DependencyManager::get<AvatarManager>();
    MyAvatar* myAvatar = manager->getMyAvatar();

    //loop through all the other avatars and simulate them...
    //gets current lookat data, removes missing avatars, etc.
    manager->updateOtherAvatars(deltaSeconds);

    myAvatar->startUpdate();
    qApp->updateMyAvatarLookAtPosition();
    // Sample hardware, update view frustum if needed, and send avatar data to mixer/nodes
    manager->updateMyAvatar(deltaSeconds);
    myAvatar->endUpdate();

    if (!isThreaded()) {
        return true;
    }
    int elapsed = (usecTimestampNow() - start);
    int usecToSleep =  _targetInterval - elapsed;
    if (usecToSleep < 0) {
        usecToSleep = 1; // always yield
    }
    usleep(usecToSleep);
    return true;
}
