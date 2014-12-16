//
//  PathUtils.cpp
//  libraries/shared/src
//
//  Created by Brad Hefta-Gaub on 12/15/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>
#include <QString>

#include "PathUtils.h"


QString& PathUtils::resourcesPath() {
#ifdef Q_OS_MAC
    static QString staticResourcePath = QCoreApplication::applicationDirPath() + "/../Resources/";
#else
    static QString staticResourcePath = QCoreApplication::applicationDirPath() + "/resources/";
#endif
    return staticResourcePath;
}
