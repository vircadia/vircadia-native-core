//
//  AvatarData.cpp
//  libraries/vircadia-client/src/avatars
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AvatarData.h"

#include <AvatarDataStream.hpp>

AvatarData::AvatarData() {
    AvatarDataStream<AvatarData>::initClientTraitsHandler();
}
