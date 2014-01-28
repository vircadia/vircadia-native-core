//
//  AvatarManager.h
//  hifi
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarManager__
#define __hifi__AvatarManager__

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include <AvatarHashMap.h>
#include <DataServerClient.h>

#include "Avatar.h"

class MyAvatar;

class AvatarManager : public QObject, public DataServerCallbackObject, public AvatarHashMap {
    Q_OBJECT
public:
    AvatarManager(QObject* parent = 0);

    void setMyAvatar(MyAvatar* myAvatar);
    
    AvatarData* getLookAtTargetAvatar() const { return _lookAtTargetAvatar.data(); }
    
    void updateLookAtTargetAvatar(glm::vec3& eyePosition);
    
    void updateAvatars(float deltaTime);
    void renderAvatars(bool forceRenderHead, bool selfAvatarOnly = false);
    
    // virtual override
    void clearHash();

public slots:
    void processDataServerResponse(const QString& userString, const QStringList& keyList, const QStringList& valueList);
    
    void processAvatarMixerDatagram(const QByteArray& datagram, const QWeakPointer<Node>& mixerWeakPointer);
    void processKillAvatar(const QByteArray& datagram);

private:
    AvatarManager(const AvatarManager& other);

    void simulateAvatarFades(float deltaTime);
    void renderAvatarFades();
    
    // virtual override
    AvatarHash::iterator erase(const AvatarHash::iterator& iterator);
    
    QWeakPointer<AvatarData> _lookAtTargetAvatar;
    glm::vec3 _lookAtOtherPosition;
    float _lookAtIndicatorScale;
    
    QVector<AvatarSharedPointer> _avatarFades;
    MyAvatar* _myAvatar;
};

#endif /* defined(__hifi__AvatarManager__) */
