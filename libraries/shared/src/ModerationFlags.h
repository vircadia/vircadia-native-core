//
//  ModerationFlags.h
//  libraries/shared/src
//
//  Created by Kalila L. on Mar 11 2021.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef vircadia_ModerationFlags_h
#define vircadia_ModerationFlags_h

class ModerationFlags {
public:
    enum BanFlags
    {
        NO_BAN = 0,
        BAN_BY_USERNAME = 1,
        BAN_BY_FINGERPRINT = 2,
        BAN_BY_IP = 4
    };
    
    static constexpr unsigned int getDefaultBanFlags() { return (BanFlags::BAN_BY_USERNAME | BanFlags::BAN_BY_FINGERPRINT); };
};

#endif // vircadia_ModerationFlags_h