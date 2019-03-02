//
//  Created by Bradley Austin Davis on 2018/10/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QCoreApplication>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include <iostream>
#include <gpu/FrameIO.h>
#include <gpu/Texture.h>


gpu::IndexOptimizer optimizer= [](gpu::Primitive primitive, uint32_t faceCount, uint32_t indexCount, uint32_t* indices ) {
    // FIXME add a triangle index optimizer here
};


void messageHandler(QtMsgType type, const QMessageLogContext &, const QString & message) {
    auto messageStr = message.toStdString();
#ifdef Q_OS_WIN
    OutputDebugStringA(messageStr.c_str());
    OutputDebugStringA("\n");
#endif
    std::cerr << messageStr << std::endl;
}

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    qInstallMessageHandler(messageHandler);
    gpu::optimizeFrame("D:/Frames/20190112_1647.json", optimizer);
    return 0;
}
