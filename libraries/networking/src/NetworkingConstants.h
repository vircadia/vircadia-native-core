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

#include <QtCore/QProcessEnvironment>
#include <QtCore/QUrl>

namespace NetworkingConstants {
    // If you want to use STAGING instead of STABLE,
    // links from the Domain Server web interface (like the connect account token generation)
    // will still point at stable unless you ALSO change the Domain Server Metaverse Server URL inside of:
    // <hifi repo>\domain-server\resources\web\js\shared.js

    // You can avoid changing that and still effectively use a connected domain on staging
    // if you manually generate a personal access token for the domains scope
    // at https://staging.highfidelity.com/user/tokens/new?for_domain_server=true

    const QUrl METAVERSE_SERVER_URL_STABLE("https://metaverse.highfidelity.com");
    const QUrl METAVERSE_SERVER_URL_STAGING("https://staging.highfidelity.com");

    // You can change the return of this function if you want to use a custom metaverse URL at compile time
    // or you can pass a custom URL via the env variable
    static const QUrl METAVERSE_SERVER_URL() {
        static const QString HIFI_METAVERSE_URL_ENV = "HIFI_METAVERSE_URL";
        static const QUrl serverURL = QProcessEnvironment::systemEnvironment().contains(HIFI_METAVERSE_URL_ENV)
            ? QUrl(QProcessEnvironment::systemEnvironment().value(HIFI_METAVERSE_URL_ENV))
            : METAVERSE_SERVER_URL_STABLE;
        return serverURL;
    };
}

#endif // hifi_NetworkingConstants_h
