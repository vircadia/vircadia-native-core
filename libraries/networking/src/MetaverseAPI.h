//
//  MetaverseAPI.h
//  libraries/networking/src
//
//  Created by Kalila L. on 2019-12-16.
//  Copyright 2019, 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef athena_MetaverseAPI_h
#define athena_MetaverseAPI_h

#include <optional>

#include <QtCore/QProcessEnvironment>
#include <QtCore/QUrl>
#include <QSslError>
#include <QNetworkReply>

namespace MetaverseAPI {

    class Settings : public QObject {
        Q_OBJECT
    public:
        static const QString GROUP;
        static const QString URL_KEY;
        static const QString URL_KEY_PATH;

        static Settings* getInstance();

        void setSettingsUrl(const QUrl& value);

    public slots:
        QUrl getBaseUrl();
        void setBaseUrl(const QUrl& value);


    protected:
        Settings(QObject* parent = nullptr);

    private:
        std::optional<QUrl> settingsURL;
    };

    QUrl getCurrentMetaverseServerURL();
    QString getCurrentMetaverseServerURLPath(bool appendForwardSlash = false);

    void logSslErrors(const QNetworkReply* reply, const QList<QSslError>& errors);
}

#endif  // athena_MetaverseAPI_h
