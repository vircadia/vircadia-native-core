//
//  common.h
//
//  Created by Nissim Hadar on 10 Nov 2017.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#ifndef hifi_common_h
#define hifi_common_h

#include <QtCore/QString>

class TestResult {
public:
    TestResult(float error, QString pathname, QString expectedImageFilename, QString actualImageFilename) :
        _error(error),
        _pathname(pathname),
        _expectedImageFilename(expectedImageFilename),
        _actualImageFilename(actualImageFilename)
    {}

    double _error;
    QString _pathname;
    QString _expectedImageFilename;
    QString _actualImageFilename;
};

enum UserResponse {
    USER_RESPONSE_INVALID,
    USER_RESPONSE_PASS,
    USE_RESPONSE_FAIL,
    USER_RESPONSE_ABORT
};

// Coefficients for luminosity calculation
const double R_Y = 0.212655f;
const double G_Y = 0.715158f;
const double B_Y = 0.072187f;

#endif // hifi_common_h