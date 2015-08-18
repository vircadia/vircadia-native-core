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
#include "InterfaceLogging.h"

enum UpdateType {
    Synchronous = 1,
    Timer,
    Thread
};

AvatarUpdate::AvatarUpdate() : _lastAvatarUpdate(0) {
    Settings settings;
    int type = settings.value("AvatarUpdateType", UpdateType::Synchronous).toInt();
    _targetSimrate = settings.value("AvatarUpdateTargetSimrate", 60).toInt();
    qCDebug(interfaceapp) << "AvatarUpdate using" << type << "at" << _targetSimrate << "sims/second";
    switch (type) {
        case UpdateType::Synchronous:
            _timer = nullptr;
            break;
        case UpdateType::Timer:
            initTimer();
            break;
        default:
            initThread();
            break;
    }
}
void AvatarUpdate::stop() {
    if (!_timer) {
        return;
    }
    _timer->stop();
    if (_avatarThread) {
        return;
    }
    _avatarThread->quit();
}

void AvatarUpdate::initTimer() {
    const qint64 AVATAR_UPDATE_INTERVAL_MSECS = 1000 / _targetSimrate;
    _timer = new QTimer(this);
    connect(_timer, &QTimer::timeout, this, &AvatarUpdate::avatarUpdate);
    _timer->start(AVATAR_UPDATE_INTERVAL_MSECS);
}
void AvatarUpdate::initThread() {
    _avatarThread = new QThread();
    _avatarThread->setObjectName("Avatar Update Thread");
    this->moveToThread(_avatarThread);
    connect(_avatarThread, &QThread::started, this, &AvatarUpdate::initTimer);
    _avatarThread->start();
}

// There are a couple of ways we could do this.
// Right now, the goals are:
// 1. allow development to switch between UpdateType
// 2. minimize changes everwhere, particularly outside of Avatars.
// As an example of the latter, we could make Application::isHMDMode() thread safe, but in this case
// we just made AvatarUpdate::isHMDMode() thread safe.
void AvatarUpdate::avatarUpdateIfSynchronous() {
    // Keep our own updated value, so that our asynchronous code can consult it.
    _isHMDMode = Application::getInstance()->isHMDMode();
    if (_updateBillboard) {
        Application::getInstance()->getMyAvatar()->doUpdateBillboard();
    }
    if (_timer) {
        return;
    }
    avatarUpdate();
}

void AvatarUpdate::avatarUpdate() {
    PerformanceTimer perfTimer("AvatarUpdate");
    quint64 now = usecTimestampNow();
    float deltaTime = (now - _lastAvatarUpdate) / (1000.0f * 1000.0f);
    Application::getInstance()->setAvatarSimrateSample(1.0f / deltaTime);
    _lastAvatarUpdate = now;
    
    //loop through all the other avatars and simulate them...
    //gets current lookat data, removes missing avatars, etc.
    DependencyManager::get<AvatarManager>()->updateOtherAvatars(deltaTime);
    
    Application::getInstance()->updateMyAvatarLookAtPosition();
    // Sample hardware, update view frustum if needed, and send avatar data to mixer/nodes
    DependencyManager::get<AvatarManager>()->updateMyAvatar(deltaTime);
}