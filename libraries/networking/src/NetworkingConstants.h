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
    const QUrl METAVERSE_SERVER_URL_STAGING { "https://staging-metaverse.vircadia.com" };

    // Web Engine requests to this parent domain have an account authorization header added
    const QString AUTH_HOSTNAME_BASE = "highfidelity.com";

    const QUrl BUILDS_XML_URL("https://highfidelity.com/builds.xml");
    const QUrl MASTER_BUILDS_XML_URL("https://highfidelity.com/dev-builds.xml");


#if USE_STABLE_GLOBAL_SERVICES
    const QString ICE_SERVER_DEFAULT_HOSTNAME = "ice.highfidelity.com";
#else
    const QString ICE_SERVER_DEFAULT_HOSTNAME = "dev-ice.highfidelity.com";
#endif

    const QString MARKETPLACE_CDN_HOSTNAME = "mpassets.highfidelity.com";

    const QUrl HELP_DOCS_URL { "https://docs.vircadia.dev" };
    const QUrl HELP_FORUM_URL { "https://forums.vircadia.dev" };
    const QUrl HELP_SCRIPTING_REFERENCE_URL{ "https://apidocs.vircadia.dev/" };
    const QUrl HELP_RELEASE_NOTES_URL{ "https://docs.vircadia.dev/release-notes.html" };
    const QUrl HELP_BUG_REPORT_URL{ "https://github.com/kasenvr/project-athena/issues" };

}

const QString HIFI_URL_SCHEME_ABOUT = "about";
const QString URL_SCHEME_HIFI = "hifi";
const QString URL_SCHEME_HIFIAPP = "hifiapp";
const QString URL_SCHEME_DATA = "data";
const QString URL_SCHEME_QRC = "qrc";
const QString HIFI_URL_SCHEME_FILE = "file";
const QString HIFI_URL_SCHEME_HTTP = "http";
const QString HIFI_URL_SCHEME_HTTPS = "https";
const QString HIFI_URL_SCHEME_FTP = "ftp";
const QString URL_SCHEME_ATP = "atp";

#endif // hifi_NetworkingConstants_h
