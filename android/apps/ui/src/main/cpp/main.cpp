
//
//  Created by Bradley Austin Davis on 2018/11/22
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <android/log.h>

#include <QtGui/QGuiApplication>

#include <Trace.h>

#include "PlayerWindow.h"

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (!message.isEmpty()) {
        const char * local=message.toStdString().c_str();
        switch (type) {
            case QtDebugMsg:
                __android_log_write(ANDROID_LOG_DEBUG,"QQ",local);
                break;
            case QtInfoMsg:
                __android_log_write(ANDROID_LOG_INFO,"QQ",local);
                break;
            case QtWarningMsg:
                __android_log_write(ANDROID_LOG_WARN,"QQ",local);
                break;
            case QtCriticalMsg:
                __android_log_write(ANDROID_LOG_ERROR,"QQ",local);
                break;
            case QtFatalMsg:
            default:
                __android_log_write(ANDROID_LOG_FATAL,"QQ",local);
                abort();
        }
    }
}

void initWebView();

int main(int argc, char** argv) {
    setupHifiApplication("uiApp");
    QGuiApplication app(argc, argv);
    initWebView();
    auto oldMessageHandler = qInstallMessageHandler(messageHandler);
    {
        DependencyManager::set<tracing::Tracer>();
        PlayerWindow window;
        app.exec();
    }
    qInstallMessageHandler(oldMessageHandler);
    return 0;
}


