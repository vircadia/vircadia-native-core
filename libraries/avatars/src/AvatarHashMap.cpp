//
//  AvatarHashMap.cpp
//  libraries/avatars/src
//
//  Created by AndrewMeadows on 1/28/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarHashMap.h"

AvatarHashMap::AvatarHashMap() :
    _avatarHash()
{
}

void AvatarHashMap::insert(const QUuid& id, AvatarSharedPointer avatar) {
    _avatarHash.insert(id, avatar);
}

AvatarHash::iterator AvatarHashMap::erase(const AvatarHash::iterator& iterator) {
    return _avatarHash.erase(iterator);
}

