//
//  Created by Bradley Austin Davis on 2018/01/11
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/qglobal.h>
#include <QtCore/QFile>

#include <QtGui/QGuiApplication>

#include <GLTFSerializer.h>
#include <shared/FileUtils.h>
#include <ResourceManager.h>
#include <DependencyManager.h>

#include <Windows.h>

// Currently only used by testing code
inline std::list<std::string> splitString(const std::string& source, const char delimiter = ' ') {
    std::list<std::string> result;
    size_t start = 0, next;

    while (std::string::npos != (next = source.find(delimiter, start))) {
        std::string sub = source.substr(start, next - start);
        if (!sub.empty()) {
            result.push_back(sub);
        }
        start = next + 1;
    }
    if (source.size() > start) {
        result.push_back(source.substr(start));
    }
    return result;
}

std::list<std::string> getGlbTestFiles() {
    return splitString(GLB_TEST_FILES, '|');
}

QtMessageHandler originalHandler;

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
#if defined(Q_OS_WIN)
    OutputDebugStringA(message.toStdString().c_str());
    OutputDebugStringA("\n");
#endif
    originalHandler(type, context, message);
}

QByteArray readFileBytes(const std::string& filename) {
    QFile file(filename.c_str());
    file.open(QFile::ReadOnly);
    QByteArray result = file.readAll();
    file.close();
    return result;
}

void processFile(const std::string& filename) {
    qDebug() << filename.c_str();
    GLTFSerializer().read(readFileBytes(filename), {}, QUrl::fromLocalFile(filename.c_str()));
}

int main(int argc, char** argv) {
    QCoreApplication app{ argc, argv };
    originalHandler = qInstallMessageHandler(messageHandler);
    
    DependencyManager::set<ResourceManager>(false);

    //processFile("c:/Users/bdavi/git/glTF-Sample-Models/2.0/Box/glTF-Binary/Box.glb");
    
    for (const auto& testFile : getGlbTestFiles()) {
        processFile(testFile);
    }
}
