//
//  MetaverseAPI.cpp
//  libraries/networking/src
//
//  Created by Kalila L. on 2019-12-16.
//  Copyright 2019, 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MetaverseAPI.h"

#include <QUrl>

#include <SettingHandle.h>
#include <DependencyManager.h>

#include "NetworkingConstants.h"
#include "NodeList.h"


namespace MetaverseAPI {

    const QString Settings::GROUP = "metaverse";
    const QString Settings::URL_KEY = "server_url";
    const QString Settings::URL_KEY_PATH = Settings::GROUP + "." + Settings::URL_KEY;

    Settings* Settings::getInstance() {
        static Settings sharedInstance;
        return &sharedInstance;
    }

    Settings::Settings(QObject* parent) : QObject(parent), settingsURL(std::nullopt) { };

    const QString BASE_URL_SETTING_KEY = "private/selectedMetaverseURL";

    std::optional<QUrl> getMetaverseURLFromDomainHandler() {
        if (DependencyManager::isSet<NodeList>()) {
            auto&& domainHandler = DependencyManager::get<NodeList>()->getDomainHandler();
            if (domainHandler.hasSettings()) {
                auto&& settings = domainHandler.getSettingsObject();
                if (settings.contains(Settings::GROUP) &&
                    settings[Settings::GROUP].isObject() &&
                    settings[Settings::GROUP].toObject().contains(Settings::URL_KEY) &&
                    settings[Settings::GROUP][Settings::URL_KEY].isString()) {
                    return QUrl(settings[Settings::GROUP][Settings::URL_KEY].toString());
                }
            }
        }

        return std::nullopt;
    }

    QUrl Settings::getBaseUrl() {
        const QString HIFI_METAVERSE_URL_ENV = "HIFI_METAVERSE_URL";

        QUrl defaultURL = NetworkingConstants::METAVERSE_SERVER_URL_STABLE;
        if (QProcessEnvironment::systemEnvironment().contains(HIFI_METAVERSE_URL_ENV)) {
            defaultURL = QUrl(QProcessEnvironment::systemEnvironment().value(HIFI_METAVERSE_URL_ENV));
        }

        auto optionalURL = settingsURL ? settingsURL : getMetaverseURLFromDomainHandler();
        QUrl settingsURL = optionalURL ? *optionalURL : defaultURL;

        return Setting::Handle<QUrl> (BASE_URL_SETTING_KEY, settingsURL).get();
    }

    void Settings::setSettingsUrl(const QUrl& value) {
        settingsURL = value;
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
