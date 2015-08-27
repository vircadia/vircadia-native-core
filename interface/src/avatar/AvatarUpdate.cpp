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

// GenericThread accepts an optional "parent" argument, defaulting to nullptr.
// This is odd, because the moveToThread() in GenericThread::initialize() will
// become a no-op if the instance ever inits QObject(someNonNullParentQObject).
// (The only clue will be a log message "QObject::moveToThread: Cannot move objects with a parent",
//  and things will end up in the same thread that created this instance. Hillarity ensues.)
// As it turns out, all the other subclasses of GenericThread (at this time) do not init
// GenericThread(parent), so things work as expected. Here we explicitly init GenericThread(nullptr)
// so that there is no confusion and no chance for a hillarious thread debugging session.
AvatarUpdate::AvatarUpdate() : GenericThread(nullptr),  _lastAvatarUpdate(0)/*, _timer(nullptr), _thread(nullptr)FIXME remove*/ {
    setObjectName("Avatar Update"); // GenericThread::initialize uses this to set the thread name.
    Settings settings;
    const int DEFAULT_TARGET_AVATAR_SIMRATE = 60;
    _targetInterval = USECS_PER_SECOND / settings.value("AvatarUpdateTargetSimrate", DEFAULT_TARGET_AVATAR_SIMRATE).toInt();
    _isToBeThreaded = settings.value("AvatarUpdateIsThreaded", true).toBool();
    if (_isToBeThreaded) {
        qCDebug(interfaceapp) << "AvatarUpdate threaded at" << _targetInterval << "microsecond interval.";
    } else {
        qCDebug(interfaceapp) << "AvatarUpdate unthreaded.";
    }
}
// We could have the constructor call initialize(), but GenericThread::initialize can take parameters.
// Keeping it separately called by the client allows that client to pass those without our
// constructor needing to know about them.

void AvatarUpdate::synchronousProcess() {

    // Keep our own updated value, so that our asynchronous code can consult it.
    _isHMDMode = Application::getInstance()->isHMDMode();
    _headPose = Application::getInstance()->getActiveDisplayPlugin()->getHeadPose();

    if (_updateBillboard) {
        Application::getInstance()->getMyAvatar()->doUpdateBillboard();
    }

    if (isThreaded()) {
        return;
    }
    process();
}

bool AvatarUpdate::process() {
    PerformanceTimer perfTimer("AvatarUpdate");
    quint64 start = usecTimestampNow();
    quint64 deltaMicroseconds = start - _lastAvatarUpdate;
    _lastAvatarUpdate = start;
    float deltaSeconds = deltaMicroseconds / (float) USECS_PER_SECOND;
    Application::getInstance()->setAvatarSimrateSample(1.0f / deltaSeconds);
    
    //loop through all the other avatars and simulate them...
    //gets current lookat data, removes missing avatars, etc.
    DependencyManager::get<AvatarManager>()->updateOtherAvatars(deltaSeconds);
    
    Application::getInstance()->getMyAvatar()->avatarLock.lockForWrite();
    Application::getInstance()->updateMyAvatarLookAtPosition();
    // Sample hardware, update view frustum if needed, and send avatar data to mixer/nodes
    DependencyManager::get<AvatarManager>()->updateMyAvatar(deltaSeconds);
    Application::getInstance()->getMyAvatar()->avatarLock.unlock();
    
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
