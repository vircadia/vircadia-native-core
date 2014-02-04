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

    void init();

    MyAvatar* getMyAvatar() { return _myAvatar.data(); }
    
    void updateOtherAvatars(float deltaTime);
    void renderAvatars(bool forceRenderHead, bool selfAvatarOnly = false);
    
    void clearOtherAvatars();

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
    
    QVector<AvatarSharedPointer> _avatarFades;
    QSharedPointer<MyAvatar> _myAvatar;
};

#endif /* defined(__hifi__AvatarManager__) */
