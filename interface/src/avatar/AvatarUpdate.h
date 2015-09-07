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
// This might get folded into AvatarManager.
class AvatarUpdate : public GenericThread {
    Q_OBJECT
public:
    AvatarUpdate();
    void synchronousProcess();
    void setRequestBillboardUpdate(bool needsUpdate) { _updateBillboard = needsUpdate; }

private:
    virtual bool process(); // No reason for other classes to invoke this.
    quint64 _lastAvatarUpdate; // microsoeconds
    quint64 _targetInterval; // microseconds
    bool _updateBillboard;

    // Goes away if Application::getActiveDisplayPlugin() and friends are made thread safe:
public:
    bool isHMDMode() { return _isHMDMode; }
    glm::mat4 getHeadPose() { return _headPose; }
private:
    bool _isHMDMode;
    glm::mat4 _headPose;
};


#endif /* defined(__hifi__AvatarUpdate__) */
