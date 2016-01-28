//
//  OpenGLInfo.h
//  interface/src
//
//  Created by David Rowe on 28 Jan 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OpenGLInfo_h
#define hifi_OpenGLInfo_h

#include <QApplication>

class OpenGLInfo : public QApplication {

public:
    OpenGLInfo(int& argc, char** argv);

    static bool isValidVersion();
};

#endif // hifi_OpenGLInfo_h
