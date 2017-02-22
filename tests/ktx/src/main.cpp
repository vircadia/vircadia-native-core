//
//  Created by Bradley Austin Davis on 2016/07/01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <QProcessEnvironment>

#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QLoggingCategory>
#include <QtCore/QRegularExpression>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QThreadPool>

#include <QtGui/QGuiApplication>
#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QApplication>

#include <shared/RateCounter.h>
#include <shared/NetworkUtils.h>
#include <shared/FileLogger.h>
#include <shared/FileUtils.h>
#include <StatTracker.h>
#include <LogHandler.h>

#include <Windows.h>
#include <gpu/Texture.h>
#include <gl/Config.h>
#include <model/TextureMap.h>
#include <ktx/KTX.h>


QSharedPointer<FileLogger> logger;

gpu::Texture* cacheTexture(const std::string& name, gpu::Texture* srcTexture, bool write = true, bool read = true);


void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    QString logMessage = LogHandler::getInstance().printMessage((LogMsgType)type, context, message);

    if (!logMessage.isEmpty()) {
#ifdef Q_OS_WIN
        OutputDebugStringA(logMessage.toLocal8Bit().constData());
        OutputDebugStringA("\n");
#endif
        logger->addMessage(qPrintable(logMessage + "\n"));
    }
}

const char * LOG_FILTER_RULES = R"V0G0N(
hifi.gpu=true
)V0G0N";

QString getRootPath() {
    static std::once_flag once;
    static QString result;
    std::call_once(once, [&] {
        QFileInfo file(__FILE__);
        QDir parent = file.absolutePath();
        result = QDir::cleanPath(parent.currentPath() + "/../../..");
    });
    return result;
}

const QString TEST_IMAGE = getRootPath() + "/scripts/developer/tests/cube_texture.png";
const QString TEST_IMAGE_KTX = getRootPath() + "/scripts/developer/tests/cube_texture.ktx";

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("KTX");
    QCoreApplication::setOrganizationName("High Fidelity");
    QCoreApplication::setOrganizationDomain("highfidelity.com");
    logger.reset(new FileLogger());

    DependencyManager::set<tracing::Tracer>();
    qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);

    QImage image(TEST_IMAGE);
    gpu::Texture* testTexture = model::TextureUsage::process2DTextureColorFromImage(image, TEST_IMAGE.toStdString(), true, false, true);

    auto ktxPtr = gpu::Texture::serialize(*testTexture);
    const auto& ktxStorage = ktxPtr->getStorage();
    auto header = ktxPtr->getHeader();
    assert(sizeof(ktx::Header) == 12 + (sizeof(uint32_t) * 13));
    QFile outFile(TEST_IMAGE_KTX);
    if (!outFile.open(QFile::Truncate | QFile::ReadWrite)) {
        throw std::runtime_error("Unable to open file");
    }
    //auto ktxSize = sizeof(ktx::Header); // ktxStorage->size()
    auto ktxSize = ktxStorage->size();
    outFile.resize(ktxSize);
    auto dest = outFile.map(0, ktxSize);
    memcpy(dest, ktxStorage->data(), ktxSize);
    outFile.unmap(dest);
    outFile.close();
//    gpu::Texture* ktxTexture = cacheTexture(TEST_IMAGE.toStdString(), testTexture, true, true);
    return 0;
}

#include "main.moc"

