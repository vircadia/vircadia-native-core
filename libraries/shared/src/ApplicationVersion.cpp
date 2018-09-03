//
//  ApplicationVersion.cpp
//  libraries/shared/src
//
//  Created by Stephen Birarda on 6/8/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ApplicationVersion.h"

#include <cassert>

#include <QtCore/QDebug>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>

ApplicationVersion::ApplicationVersion(const QString& versionString) :
    versionString(versionString)
{
    // attempt to regex out a semantic version from the string
    // handling both x.y.z and x.y formats
    QRegExp semanticRegex("([\\d]+)\\.([\\d]+)(?:\\.([\\d]+))?");

    int pos = semanticRegex.indexIn(versionString);
    if (pos != -1) {
        isSemantic = true;
        auto captures = semanticRegex.capturedTexts();

        major = captures[1].toInt();
        minor = captures[2].toInt();

        if (captures.length() > 3) {
            patch = captures[3].toInt();
        } else {
            // the patch is implictly 0 if it was not included
            patch = 0;
        }
    } else {
        // if we didn't have a sematic style, we assume that we just have a build number
        build = versionString.toInt();
    }
}

bool ApplicationVersion::operator==(const ApplicationVersion& other) const {
    if (isSemantic && other.isSemantic) {
        return major == other.major && minor == other.minor && patch == other.patch;
    } else if (!isSemantic && !other.isSemantic) {
        return build == other.build;
    } else {
        assert(isSemantic == other.isSemantic);
        return false;
    }
}

bool ApplicationVersion::operator<(const ApplicationVersion& other) const {
    if (isSemantic && other.isSemantic) {
        if (major == other.major) {
            if (minor == other.minor) {
                return patch < other.patch;
            } else {
                return minor < other.minor;
            }
        } else {
            return major < other.major;
        }
    } else if (!isSemantic && !other.isSemantic) {
        return build < other.build;
    } else {
        assert(isSemantic == other.isSemantic);
        return false;
    }
}

bool ApplicationVersion::operator>(const ApplicationVersion& other) const {
    if (isSemantic && other.isSemantic) {
        if (major == other.major) {
            if (minor == other.minor) {
                return patch > other.patch;
            } else {
                return minor > other.minor;
            }
        } else {
            return major > other.major;
        }
    } else if (!isSemantic && !other.isSemantic) {
        return build > other.build;
    } else {
        assert(isSemantic == other.isSemantic);
        return false;
    }
}
