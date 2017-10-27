//
//  Created by Bradley Austin Davis on 2017/04/27
//  Copyright 2013-2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OtherAvatar_h
#define hifi_OtherAvatar_h

#include "Avatar.h"

class OtherAvatar : public Avatar {
public:
    explicit OtherAvatar(QThread* thread);
    virtual void instantiableAvatar() override {};
};

#endif // hifi_OtherAvatar_h
