//
//  AvatarHashMap.h
//  libraries/avatars/src
//
//  Created by Stephen AndrewMeadows on 1/28/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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
