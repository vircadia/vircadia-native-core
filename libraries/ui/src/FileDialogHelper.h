//
//  Created by Bradley Austin Davis on 2016-01-21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once
#ifndef hifi_ui_FileDialogHelper_h
#define hifi_ui_FileDialogHelper_h

#include <QtCore/QObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QUrl>
#include <QtCore/QString>
#include <QtCore/QStringList>


class FileDialogHelper : public QObject {
    Q_OBJECT
    Q_ENUMS(StandardLocation)

public:
    // Do not re-order, must match QDesktopServices
    enum StandardLocation {
        DesktopLocation,
        DocumentsLocation,
        FontsLocation,
        ApplicationsLocation,
        MusicLocation,
        MoviesLocation,
        PicturesLocation,
        TempLocation,
        HomeLocation,
        DataLocation,
        CacheLocation,
        GenericDataLocation,
        RuntimeLocation,
        ConfigLocation,
        DownloadLocation,
        GenericCacheLocation,
        GenericConfigLocation,
        AppDataLocation,
        AppConfigLocation,
        AppLocalDataLocation = DataLocation
    };

    Q_INVOKABLE QUrl home();
    Q_INVOKABLE QStringList standardPath(StandardLocation location);
    Q_INVOKABLE QString urlToPath(const QUrl& url);
    Q_INVOKABLE bool validPath(const QString& path);
    Q_INVOKABLE bool validFolder(const QString& path);
    Q_INVOKABLE QUrl pathToUrl(const QString& path);
};


#endif
