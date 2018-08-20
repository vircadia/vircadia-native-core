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

    const QUrl METAVERSE_SERVER_URL_STABLE { "https://metaverse.highfidelity.com" };
    const QUrl METAVERSE_SERVER_URL_STAGING { "https://staging.highfidelity.com" };
    QUrl METAVERSE_SERVER_URL();
}

const QString URL_SCHEME_ABOUT = "about";
const QString URL_SCHEME_HIFI = "hifi";
const QString URL_SCHEME_QRC = "qrc";
const QString URL_SCHEME_FILE = "file";
const QString URL_SCHEME_HTTP = "http";
const QString URL_SCHEME_HTTPS = "https";
const QString URL_SCHEME_FTP = "ftp";
const QString URL_SCHEME_ATP = "atp";

#endif // hifi_NetworkingConstants_h
