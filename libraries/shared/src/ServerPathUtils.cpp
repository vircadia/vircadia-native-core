//
//  ServerPathUtils.cpp
//  libraries/shared/src
//
//  Created by Ryan Huffman on 01/12/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "ServerPathUtils.h"

#include <QStandardPaths>
#include <QtCore/QDir>
#include <QtWidgets/qapplication.h>
#include <QDebug>

QString ServerPathUtils::getDataDirectory() {
    auto homeDirectory = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));

#ifdef Q_OS_WIN
    homeDirectory.cd("AppData/Roaming/");
#elif Q_OS_OSX
    homeDirectory.cd("Library/Application Support/");
#else
    homeDirectory.cd(".local/share/");
#endif

    homeDirectory.cd(qApp->organizationName() + "/" + qApp->applicationName());

    return homeDirectory.absolutePath();
}

QString ServerPathUtils::getDataFilePath(QString filename) {
    return QDir(getDataDirectory()).absoluteFilePath(filename);
}

