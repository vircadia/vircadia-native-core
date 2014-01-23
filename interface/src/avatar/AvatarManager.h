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

#include "Avatar.h"

class AvatarManager : public QObject {
    Q_OBJECT
public:
    AvatarManager(QObject* parent = 0);
public slots:
    void processAvatarMixerDatagram(const QByteArray& datagram);
    void processKillAvatar(const QByteArray& datagram);
    void clearHash();
private:
    QHash<QUuid, Avatar*> _hash;
};

#endif /* defined(__hifi__AvatarManager__) */
