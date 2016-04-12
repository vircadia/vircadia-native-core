//
//  OpenGLVersionChecker.h
//  libraries/gl/src/gl
//
//  Created by David Rowe on 28 Jan 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OpenGLVersionChecker_h
#define hifi_OpenGLVersionChecker_h

#include <QApplication>

class OpenGLVersionChecker : public QApplication {

public:
    OpenGLVersionChecker(int& argc, char** argv);

    static QJsonObject checkVersion(bool& valid, bool& override);
};

#endif // hifi_OpenGLVersionChecker_h
