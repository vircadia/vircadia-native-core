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
    const QStringList IS_AUTHABLE_HOSTNAME = { "highfidelity.com", "highfidelity.io" };
    
    // Use a custom User-Agent to avoid ModSecurity filtering, e.g. by hosting providers.
    const QByteArray VIRCADIA_USER_AGENT = "Mozilla/5.0 (VircadiaInterface)";
    
    const QString WEB_ENGINE_USER_AGENT = "Chrome/48.0 (VircadiaInterface)";
    const QString METAVERSE_USER_AGENT = "Chrome/48.0 (VircadiaInterface)";
    const QString MOBILE_USER_AGENT = "Mozilla/5.0 (Linux; Android 6.0; Nexus 5 Build/MRA58N) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/56.0.2924.87 Mobile Safari/537.36";

    const QUrl BUILDS_XML_URL("https://highfidelity.com/builds.xml");
    const QUrl MASTER_BUILDS_XML_URL("https://highfidelity.com/dev-builds.xml");
    
    // WebEntity Defaults
    const QString WEB_ENTITY_DEFAULT_SOURCE_URL = "https://vircadia.com/";
    
    const QString DEFAULT_AVATAR_COLLISION_SOUND_URL = "https://hifi-public.s3.amazonaws.com/sounds/Collisions-otherorganic/Body_Hits_Impact.wav";

    // CDN URLs
    const QString S3_URL = "http://s3.amazonaws.com/hifi-public";
    const QString PUBLIC_URL = "http://public.highfidelity.io";

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
