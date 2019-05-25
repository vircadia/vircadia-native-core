//
//  Created by Bradley Austin Davis on 2018/11/22
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <android/log.h>

#include <QtGui/QGuiApplication>
#include <QtCore/QTimer>
#include <QtCore/QFileInfo>
#include <QtAndroidExtras/QAndroidJniObject>

#include <Trace.h>

#include "PlayerWindow.h"
#include "AndroidHelper.h"


void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (!message.isEmpty()) {
        const char* local = message.toStdString().c_str();
        switch (type) {
            case QtDebugMsg:
                __android_log_write(ANDROID_LOG_DEBUG, "Interface", local);
                break;
            case QtInfoMsg:
                __android_log_write(ANDROID_LOG_INFO, "Interface", local);
                break;
            case QtWarningMsg:
                __android_log_write(ANDROID_LOG_WARN, "Interface", local);
                break;
            case QtCriticalMsg:
                __android_log_write(ANDROID_LOG_ERROR, "Interface", local);
                break;
            case QtFatalMsg:
            default:
                __android_log_write(ANDROID_LOG_FATAL, "Interface", local);
                abort();
        }
    }
}

int main(int argc, char** argv) {
    setupHifiApplication("gpuFramePlayer");
    QGuiApplication app(argc, argv);
    auto oldMessageHandler = qInstallMessageHandler(messageHandler);
    DependencyManager::set<tracing::Tracer>();
    PlayerWindow window;
    QTimer::singleShot(10, []{
        __android_log_write(ANDROID_LOG_WARN, "QQQ", "notifyLoadComplete");
        AndroidHelper::instance().notifyLoadComplete();
    });
    __android_log_write(ANDROID_LOG_WARN, "QQQ", "Exec");
    app.exec();
    __android_log_write(ANDROID_LOG_WARN, "QQQ", "Exec done");
    qInstallMessageHandler(oldMessageHandler);
    return 0;
}
