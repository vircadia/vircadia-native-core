//
//  AvatarData.h
//  libraries/vircadia-client/src/avatars
//
//  Created by Nshan G. on 9 May 2022.
//  Copyright 2022 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_AVATARDATA_H
#define LIBRARIES_VIRCADIA_CLIENT_SRC_INTERNAL_AVATARS_AVATARDATA_H

#include <QObject>

#include <AvatarDataStream.h>

class AvatarData : public QObject, public AvatarDataStream<AvatarData> {
    Q_OBJECT
    public:
        AvatarData();
};

#endif /* end of include guard */
