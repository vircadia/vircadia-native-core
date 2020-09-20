//
//  MetaverseAPI.cpp
//  libraries/networking/src
//
//  Created by Kalila L. on 2019-12-16.
//  Copyright 2019 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MetaverseAPI.h"

#include <QUrl>
#include <QDebug>
#include "NetworkingConstants.h"
#include <SettingHandle.h>


namespace MetaverseAPI {
    // You can change the return of this function if you want to use a custom metaverse URL at compile time
    // or you can pass a custom URL via the env variable
    QUrl getCurrentMetaverseServerURL() {
        QUrl selectedMetaverseURL;
        Setting::Handle<QUrl> selectedMetaverseURLSetting("private/selectedMetaverseURL",
                                                          NetworkingConstants::METAVERSE_SERVER_URL_STABLE);

        selectedMetaverseURL = selectedMetaverseURLSetting.get();

        const QString HIFI_METAVERSE_URL_ENV = "HIFI_METAVERSE_URL";

        if (QProcessEnvironment::systemEnvironment().contains(HIFI_METAVERSE_URL_ENV)) {
            return QUrl(QProcessEnvironment::systemEnvironment().value(HIFI_METAVERSE_URL_ENV));
        }

        return selectedMetaverseURL;
    };

    QString getCurrentMetaverseServerURLPath(bool appendForwardSlash){ 
        QString path = getCurrentMetaverseServerURL().path();

        if (!path.isEmpty() && appendForwardSlash) {
            path.append("/");
        }

        return path;
    };
}  // namespace MetaverseAPI
