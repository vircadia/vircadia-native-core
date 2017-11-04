//
//  NetworkingConstants.h
//  libraries/networking/src
//
//  Created by Stephen Birarda on 2015-03-31.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_NetworkingConstants_h
#define hifi_NetworkingConstants_h

#include <QtCore/QUrl>

namespace NetworkingConstants {
    // If you want to use STAGING instead of STABLE,
    //     don't forget to ALSO change the Domain Server Metaverse Server URL inside of:
    // <hifi repo>\domain-server\resources\web\js\shared.js
    const QUrl METAVERSE_SERVER_URL_STABLE("https://metaverse.highfidelity.com");
    const QUrl METAVERSE_SERVER_URL_STAGING("https://staging.highfidelity.com");
    const QUrl METAVERSE_SERVER_URL = METAVERSE_SERVER_URL_STABLE;
}

#endif // hifi_NetworkingConstants_h
