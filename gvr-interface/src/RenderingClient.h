//
//  RenderingClient.h
//  gvr-interface/src
//
//  Created by Stephen Birarda on 1/20/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_RenderingClient_h
#define hifi_RenderingClient_h

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QTimer>

#include <AvatarData.h>

#include "Client.h"

class RenderingClient : public Client {
    Q_OBJECT
public:
    RenderingClient(QObject* parent = 0, const QString& launchURLString = QString());
    
    const glm::vec3& getPosition() const { return _position; }
    const glm::quat& getOrientation() const { return _orientation; }
    void setOrientation(const glm::quat& orientation) { _orientation = orientation; }
    
    static glm::vec3 getPositionForAudio() { return _instance->getPosition(); }
    static glm::quat getOrientationForAudio() { return _instance->getOrientation(); }
    
    virtual void cleanupBeforeQuit();
    
private slots:
    void goToLocation(const glm::vec3& newPosition,
                      bool hasOrientationChange, const glm::quat& newOrientation,
                      bool shouldFaceLocation);
    void sendAvatarPacket();
    
private:
    virtual void processVerifiedPacket(const HifiSockAddr& senderSockAddr, const QByteArray& incomingPacket);
    
    static RenderingClient* _instance;
    
    glm::vec3 _position;
    glm::quat _orientation;

    QTimer _avatarTimer;
    AvatarData _fakeAvatar;
};

#endif // hifi_RenderingClient_h
