//
//  AvatarHashMap.cpp
//  hifi
//
//  Created by AndrewMeadows on 1/28/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include "AvatarHashMap.h"

AvatarHashMap::AvatarHashMap() :
    _avatarHash()
{
}

void AvatarHashMap::insert(const QUuid& id, AvatarSharedPointer avatar) {
    _avatarHash.insert(id, avatar);
    avatar->setSessionUUID(id);
}

AvatarHash::iterator AvatarHashMap::erase(const AvatarHash::iterator& iterator) {
    return _avatarHash.erase(iterator);
}

