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

#include <vector>
#include <QtCore/QString>

class SSIMResults {
public:
    int width;
    int height;
    std::vector<double> results;
 
    double ssim;
    double worstTileValue;

    // Used for scaling
    double min;
    double max;
};

class TestResult {
public:
    TestResult(double errorGlobal, double errorLocal, const QString& pathname, const QString& expectedImageFilename, const QString& actualImageFilename, const SSIMResults& ssimResults) :
        _errorGlobal(errorGlobal),
        _errorLocal(errorLocal),
        _pathname(pathname),
        _expectedImageFilename(expectedImageFilename),
        _actualImageFilename(actualImageFilename),
        _ssimResults(ssimResults)
    {}

    double _errorGlobal;
    double _errorLocal;

    QString _pathname;
    QString _expectedImageFilename;
    QString _actualImageFilename;

    SSIMResults _ssimResults;
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

const QString nitpickVersion{ "v3.2.0" }; 
#endif // hifi_common_h