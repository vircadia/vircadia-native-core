//
//  OffscreenUi.cpp
//  interface/src/render-utils
//
//  Created by Bradley Austin Davis on 2015-04-04
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "FileDialogHelper.h"

#include <QtCore/QDir>
#include <QtCore/QFile>


QUrl FileDialogHelper::home() {
    return pathToUrl(QStandardPaths::standardLocations(QStandardPaths::HomeLocation)[0]);
}

QStringList FileDialogHelper::standardPath(StandardLocation location) {
    return QStandardPaths::standardLocations(static_cast<QStandardPaths::StandardLocation>(location)); 
}

QString FileDialogHelper::urlToPath(const QUrl& url) { 
    return url.toLocalFile(); 
}

bool FileDialogHelper::validPath(const QString& path) { 
    return QFile(path).exists(); 
}

bool FileDialogHelper::validFolder(const QString& path) { 
    return QDir(path).exists(); 
}

QUrl FileDialogHelper::pathToUrl(const QString& path) { 
    return QUrl::fromLocalFile(path); 
}

