//
//  Created by Bradley Austin Davis on 2018/05/13
//  Copyright 2013-2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Utils.h"

#include <QtCore/qlogging.h>
#include <QtCore/QString>
#include <QtCore/QByteArray>

#if defined(Q_OS_WIN)
#include <Windows.h>
#endif


#include "FileDownloader.h"

static QtMessageHandler originalHandler;

static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
#if defined(Q_OS_WIN)
    OutputDebugStringA(message.toStdString().c_str());
    OutputDebugStringA("\n");
#endif
    originalHandler(type, context, message);
}


void installTestMessageHandler() {
    originalHandler = qInstallMessageHandler(messageHandler);
}

bool downloadFile(const QString& file, const std::function<void(const QByteArray&)> handler) {
    FileDownloader(file, handler).waitForDownload();
    return true;
}