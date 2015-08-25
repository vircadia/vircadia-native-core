//
//  AvatarUpdate.h
//  interface/src/avatar
//
//  Created by Howard Stearns on 8/18/15.
///
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//

#ifndef __hifi__AvatarUpdate__
#define __hifi__AvatarUpdate__

#include <QtCore/QObject>
#include <QTimer>

// There are a couple of ways we could do this.
// Right now, the goals are:
// 1. allow development to switch between UpdateType
//   e.g.: see uses of UpdateType.
// 2. minimize changes everwhere, particularly outside of Avatars.
//   e.g.: we could make Application::isHMDMode() thread safe, but for now we just made AvatarUpdate::isHMDMode() thread safe.


// Home for the avatarUpdate operations (e.g., whether on a separate thread, pipelined in various ways, etc.)
// This might get folded into AvatarManager.
class AvatarUpdate : public QObject {
    Q_OBJECT
public:
    AvatarUpdate();
    void synchronousProcess();
    void setRequestBillboardUpdate(bool needsUpdate) { _updateBillboard = needsUpdate; }
    void terminate(); // Extends GenericThread::terminate to also kill timer.

private:
    virtual bool process(); // No reason for other classes to invoke this.
    void initTimer();
    int _targetSimrate;
    bool _updateBillboard;
    QTimer* _timer;
    quint64 _lastAvatarUpdate;

    // Goes away when we get rid of the ability to switch back and forth in settings:
    enum UpdateType {
        Synchronous = 1,
        Timer,
        Thread
    };
    int _type;
public:
    bool isToBeThreaded() const { return _type == UpdateType::Thread; } // Currently set by constructor from settings.
    void threadRoutine();

    // Goes away when using GenericThread:
    void initialize(bool isThreaded);
private:
    QThread* _thread;
    void initThread();

    // Goes away if Applicaiton::isHMDMode() is made thread safe:
public:
    bool isHMDMode() { return _isHMDMode; }
private:
    bool _isHMDMode;
};


#endif /* defined(__hifi__AvatarUpdate__) */
