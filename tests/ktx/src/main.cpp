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


#include <gpu/Texture.h>
#include <gl/Config.h>
#include <model/TextureMap.h>
#include <ktx/KTX.h>
#include <image/Image.h>


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

    Q_ASSERT(sizeof(ktx::Header) == 12 + (sizeof(uint32_t) * 13));

    DependencyManager::set<tracing::Tracer>();
    qInstallMessageHandler(messageHandler);
    QLoggingCategory::setFilterRules(LOG_FILTER_RULES);

    QImage image(TEST_IMAGE);
    gpu::TexturePointer testTexture = image::TextureUsage::process2DTextureColorFromImage(image, TEST_IMAGE.toStdString(), true);

    auto ktxMemory = gpu::Texture::serialize(*testTexture);
    {
        const auto& ktxStorage = ktxMemory->getStorage();
        QFile outFile(TEST_IMAGE_KTX);
        if (!outFile.open(QFile::Truncate | QFile::ReadWrite)) {
            throw std::runtime_error("Unable to open file");
        }
        auto ktxSize = ktxStorage->size();
        outFile.resize(ktxSize);
        auto dest = outFile.map(0, ktxSize);
        memcpy(dest, ktxStorage->data(), ktxSize);
        outFile.unmap(dest);
        outFile.close();
    }

    {
        auto ktxFile = ktx::KTX::create(std::shared_ptr<storage::Storage>(new storage::FileStorage(TEST_IMAGE_KTX)));
        {
            const auto& memStorage = ktxMemory->getStorage();
            const auto& fileStorage = ktxFile->getStorage();
            Q_ASSERT(memStorage->size() == fileStorage->size());
            Q_ASSERT(memStorage->data() != fileStorage->data());
            Q_ASSERT(0 == memcmp(memStorage->data(), fileStorage->data(), memStorage->size()));
            Q_ASSERT(ktxFile->_images.size() == ktxMemory->_images.size());
            auto imageCount = ktxFile->_images.size();
            auto startMemory = ktxMemory->_storage->data();
            auto startFile = ktxFile->_storage->data();
            for (size_t i = 0; i < imageCount; ++i) {
                auto memImages = ktxMemory->_images[i];
                auto fileImages = ktxFile->_images[i];
                Q_ASSERT(memImages._padding == fileImages._padding);
                Q_ASSERT(memImages._numFaces == fileImages._numFaces);
                Q_ASSERT(memImages._imageSize == fileImages._imageSize);
                Q_ASSERT(memImages._faceSize == fileImages._faceSize);
                Q_ASSERT(memImages._faceBytes.size() == memImages._numFaces);
                Q_ASSERT(fileImages._faceBytes.size() == fileImages._numFaces);
                auto faceCount = fileImages._numFaces;
                for (uint32_t face = 0; face < faceCount; ++face) {
                    auto memFace = memImages._faceBytes[face];
                    auto memOffset = memFace - startMemory;
                    auto fileFace = fileImages._faceBytes[face];
                    auto fileOffset = fileFace - startFile;
                    Q_ASSERT(memOffset % 4 == 0);
                    Q_ASSERT(memOffset == fileOffset);
                }
            }
        }
    }
    testTexture->setKtxBacking(TEST_IMAGE_KTX.toStdString());
    return 0;
}

#include "main.moc"

