//
//  FileUtils.h
//  interface/src
//
//  Created by Stojce Slavkovski on 12/23/13.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2022 Overte e.V.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileUtils_h
#define hifi_FileUtils_h

#include "../QtCompatibility.h"
#include <QtCore/QString>

class FileUtils {

public:
    static const QStringList& getFileSelectors();
    static QString selectFile(const QString& fileName);
    static void locateFile(const QString& fileName);
    static bool exists(const QString& fileName);
    static bool isRelative(const QString& fileName);
    static QString standardPath(QString subfolder);
    static QString readFile(const QString& filename);
    static QStringList readLines(const QString& filename, QTCOMPAT_SPLIT_BEHAVIOR splitBehavior = QTCOMPAT_KEEP_EMPTY_PARTS);
    static QString replaceDateTimeTokens(const QString& path);
    static QString computeDocumentPath(const QString& path);
    static bool canCreateFile(const QString& fullPath);
    static QString getParentPath(const QString& fullPath);
};

#endif // hifi_FileUtils_h
