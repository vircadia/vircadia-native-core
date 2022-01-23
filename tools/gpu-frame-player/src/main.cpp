//
//  Created by Bradley Austin Davis on 2016/07/01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtWidgets/QApplication>
#include <QtCore/QSharedPointer>

#include <shared/FileLogger.h>
#include "PlayerWindow.h"

Q_DECLARE_LOGGING_CATEGORY(gpu_player_logging)
Q_LOGGING_CATEGORY(gpu_player_logging, "hifi.gpu.player")

QSharedPointer<FileLogger> logger;

static const QString LAST_FRAME_FILE = "lastFrameFile";

static void setup() { 
    DependencyManager::set<tracing::Tracer>(); 
}

int main(int argc, char** argv) {
    setupHifiApplication("gpuFramePlayer");

    QApplication app(argc, argv);
    logger.reset(new FileLogger());
    setup();
    PlayerWindow window;
    app.exec();
    return 0;
}
