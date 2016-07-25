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
    Q_INVOKABLE QStringList drives();
    Q_INVOKABLE QString urlToPath(const QUrl& url);
    Q_INVOKABLE bool urlIsDir(const QUrl& url);
    Q_INVOKABLE bool urlIsFile(const QUrl& url);
    Q_INVOKABLE bool urlExists(const QUrl& url);
    Q_INVOKABLE bool urlIsWritable(const QUrl& url);
    Q_INVOKABLE bool fileExists(const QString& path);
    Q_INVOKABLE bool validPath(const QString& path);
    Q_INVOKABLE bool validFolder(const QString& path);
    Q_INVOKABLE QUrl pathToUrl(const QString& path);
    Q_INVOKABLE QUrl saveHelper(const QString& saveText, const QUrl& currentFolder, const QStringList& selectionFilters);
    Q_INVOKABLE QList<QUrl> urlToList(const QUrl& url);

    Q_INVOKABLE void openDirectory(const QString& path);
};


#endif
