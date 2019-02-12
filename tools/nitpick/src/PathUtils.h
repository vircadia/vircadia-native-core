//
//  PathUtils.h
//
//  Created by Nissim Hadar on 11 Feb 2019.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_PathUtils_h
#define hifi_PathUtils_h

#include <Qstring>

class PathUtils {
public:
    static QString getPathToExecutable(const QString& executableName);
};

#endif