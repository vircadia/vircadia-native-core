//
//  QOpenGLDebugLoggerWrapper.cpp
//
//
//  Created by Clement on 12/4/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QOpenGLDebugLoggerWrapper.h"

#include <QObject>
#include <QOpenGLDebugLogger>

void OpenGLDebug::log(const QOpenGLDebugMessage & debugMessage) {
    qDebug() << debugMessage;
}

void setupDebugLogger(QObject* window) {
    QOpenGLDebugLogger* logger = new QOpenGLDebugLogger(window);
    logger->initialize(); // initializes in the current context, i.e. ctx
    logger->enableMessages();
    QObject::connect(logger, &QOpenGLDebugLogger::messageLogged, window, [&](const QOpenGLDebugMessage & debugMessage) {
        OpenGLDebug::log(debugMessage);
        
    });
}