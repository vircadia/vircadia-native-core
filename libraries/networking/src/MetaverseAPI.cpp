//
//  MetaverseAPI.cpp
//  libraries/networking/src
//
//  Created by Kalila L. on 2019-12-16.
//  Copyright 2019, 2022 Vircadia contributors.
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

    Settings* Settings::getInstance() {
        static Settings sharedInstance;
        return &sharedInstance;
    }

    Settings::Settings(QObject* parent) : QObject(parent) { };

    const QString BASE_URL_SETTING_KEY = "private/selectedMetaverseURL";

    QUrl Settings::getBaseUrl() {
        const QString HIFI_METAVERSE_URL_ENV = "HIFI_METAVERSE_URL";

        QUrl defaultURL = NetworkingConstants::METAVERSE_SERVER_URL_STABLE;
        if (QProcessEnvironment::systemEnvironment().contains(HIFI_METAVERSE_URL_ENV)) {
            defaultURL = QUrl(QProcessEnvironment::systemEnvironment().value(HIFI_METAVERSE_URL_ENV));
        }

        return Setting::Handle<QUrl> (BASE_URL_SETTING_KEY, defaultURL).get();
    }

    void Settings::setBaseUrl(const QUrl& value) {
        if (getBaseUrl() == value) {
            return;
        }

        Setting::Handle<QVariant> handle(BASE_URL_SETTING_KEY);

        if (value.isValid() && !value.isEmpty()) {
            handle.set(value);
        } else {
            handle.set(QVariant{}); // this removes, while handle.remove() doesn't :/
        }
    }

    QUrl getCurrentMetaverseServerURL() {
        return Settings::getInstance()->getBaseUrl();
    };

    QString getCurrentMetaverseServerURLPath(bool appendForwardSlash){
        QString path = getCurrentMetaverseServerURL().path();

        if (!path.isEmpty() && appendForwardSlash) {
            path.append("/");
        }

        return path;
    };
}  // namespace MetaverseAPI
