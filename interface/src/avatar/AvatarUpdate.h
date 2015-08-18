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

// Home for the avatarUpdate operations (e.g., whether on a separate thread, pipelined in various ways, etc.)
// TODO: become GenericThread
// This might get folded into AvatarManager.
class AvatarUpdate : public QObject {
    Q_OBJECT
public:
    AvatarUpdate();
    void avatarUpdateIfSynchronous();
    bool isHMDMode() { return _isHMDMode; }
    void setRequestBillboardUpdate(bool needsUpdate) { _updateBillboard = needsUpdate; }
    void stop();
private:
    void initTimer();
    void initThread();
    void avatarUpdate();
    int _targetSimrate;
    bool _isHMDMode;
    bool _updateBillboard;
    QTimer* _timer;
    QThread* _avatarThread;
    quint64 _lastAvatarUpdate;
};


#endif /* defined(__hifi__AvatarUpdate__) */
