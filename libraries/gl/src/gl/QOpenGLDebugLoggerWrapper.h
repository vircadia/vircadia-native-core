//
//  QOpenGLDebugLoggerWrapper.h
//
//
//  Created by Clement on 12/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QOpenGLDebugLoggerWrapper_h
#define hifi_QOpenGLDebugLoggerWrapper_h

class QObject;
class  QOpenGLDebugMessage;

void setupDebugLogger(QObject* window);

class OpenGLDebug {
public:
    static void log(const QOpenGLDebugMessage & debugMessage);
};

#endif // hifi_QOpenGLDebugLoggerWrapper_h