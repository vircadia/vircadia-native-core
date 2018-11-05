//
//  ApplicationVersion.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 6/8/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ApplicationVersion_h
#define hifi_ApplicationVersion_h

#include <QtCore/QString>

class ApplicationVersion {
public:
    ApplicationVersion(const QString& versionString);

    bool operator==(const ApplicationVersion& other) const;
    bool operator!=(const ApplicationVersion& other) const { return !(*this == other); }

    bool operator <(const ApplicationVersion& other) const;
    bool operator >(const ApplicationVersion& other) const;

    bool operator >=(const ApplicationVersion& other) const { return (*this == other) || (*this > other); }
    bool operator <=(const ApplicationVersion& other) const { return (*this == other) || (*this < other); }

    int major = -1;
    int minor = -1;
    int patch = -1;

    int build = -1;

    bool isSemantic { false };

    QString versionString;
};

#endif // hifi_ApplicationVersion_h
