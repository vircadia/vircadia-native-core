//
//  AvatarHashMap.h
//  hifi
//
//  Created by Stephen AndrewMeadows on 1/28/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarHashMap__
#define __hifi__AvatarHashMap__

#include <QtCore/QHash>
#include <QtCore/QSharedPointer>
#include <QtCore/QUuid>

#include "AvatarData.h"

typedef QSharedPointer<AvatarData> AvatarSharedPointer;
typedef QHash<QUuid, AvatarSharedPointer> AvatarHash;

class AvatarHashMap {
public:
    AvatarHashMap();
    
    const AvatarHash& getAvatarHash() { return _avatarHash; }
    int size() const { return _avatarHash.size(); }

    virtual void insert(const QUuid& id, AvatarSharedPointer avatar);

protected:
    virtual AvatarHash::iterator erase(const AvatarHash::iterator& iterator);

    AvatarHash _avatarHash;
};

#endif /* defined(__hifi__AvatarHashMap__) */
