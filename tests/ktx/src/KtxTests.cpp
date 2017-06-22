//
//  Created by Bradley Austin Davis on 2016/07/01
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "KtxTests.h"

#include <mutex>

#include <QtTest/QtTest>

#include <ktx/KTX.h>
#include <gpu/Texture.h>
#include <image/Image.h>


QTEST_GUILESS_MAIN(KtxTests)

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

void KtxTests::initTestCase() {
}

void KtxTests::cleanupTestCase() {
}

void KtxTests::testKhronosCompressionFunctions() {
    using namespace khronos::gl::texture;
    QCOMPARE(evalAlignedCompressedBlockCount<4>(0), (uint32_t)0x0);
    QCOMPARE(evalAlignedCompressedBlockCount<4>(1), (uint32_t)0x1);
    QCOMPARE(evalAlignedCompressedBlockCount<4>(4), (uint32_t)0x1);
    QCOMPARE(evalAlignedCompressedBlockCount<4>(5), (uint32_t)0x2);
    QCOMPARE(evalCompressedBlockCount(InternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT, 0x00), (uint32_t)0x00);
    QCOMPARE(evalCompressedBlockCount(InternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT, 0x01), (uint32_t)0x01);
    QCOMPARE(evalCompressedBlockCount(InternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT, 0x04), (uint32_t)0x01);
    QCOMPARE(evalCompressedBlockCount(InternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT, 0x05), (uint32_t)0x02);
    QCOMPARE(evalCompressedBlockCount(InternalFormat::COMPRESSED_SRGB_S3TC_DXT1_EXT, 0x1000), (uint32_t)0x400);

    QVERIFY_EXCEPTION_THROWN(evalCompressedBlockCount(InternalFormat::RGBA8, 0x00), std::runtime_error);
}

void KtxTests::testKtxEvalFunctions() {
    QCOMPARE(sizeof(ktx::Header), (size_t)64);
    QCOMPARE(ktx::evalPadding(0x0), (uint8_t)0);
    QCOMPARE(ktx::evalPadding(0x1), (uint8_t)3);
    QCOMPARE(ktx::evalPadding(0x2), (uint8_t)2);
    QCOMPARE(ktx::evalPadding(0x3), (uint8_t)1);
    QCOMPARE(ktx::evalPadding(0x4), (uint8_t)0);
    QCOMPARE(ktx::evalPadding(0x400), (uint8_t)0);
    QCOMPARE(ktx::evalPadding(0x401), (uint8_t)3);
    QCOMPARE(ktx::evalPaddedSize(0x0), 0x0);
    QCOMPARE(ktx::evalPaddedSize(0x1), 0x4);
    QCOMPARE(ktx::evalPaddedSize(0x2), 0x4);
    QCOMPARE(ktx::evalPaddedSize(0x3), 0x4);
    QCOMPARE(ktx::evalPaddedSize(0x4), 0x4);
    QCOMPARE(ktx::evalPaddedSize(0x400), 0x400);
    QCOMPARE(ktx::evalPaddedSize(0x401), 0x404);
    QCOMPARE(ktx::evalAlignedCount((uint32_t)0x0), (uint32_t)0x0);
    QCOMPARE(ktx::evalAlignedCount((uint32_t)0x1), (uint32_t)0x1);
    QCOMPARE(ktx::evalAlignedCount((uint32_t)0x4), (uint32_t)0x1);
    QCOMPARE(ktx::evalAlignedCount((uint32_t)0x5), (uint32_t)0x2);
}

void KtxTests::testKtxSerialization() {
    const QString TEST_IMAGE = getRootPath() + "/scripts/developer/tests/cube_texture.png";
    QImage image(TEST_IMAGE);
    gpu::TexturePointer testTexture = image::TextureUsage::process2DTextureColorFromImage(image, TEST_IMAGE.toStdString(), true);
    auto ktxMemory = gpu::Texture::serialize(*testTexture);
    QVERIFY(ktxMemory.get());

    // Serialize the image to a file
    QTemporaryFile TEST_IMAGE_KTX;
    {
        const auto& ktxStorage = ktxMemory->getStorage();
        QVERIFY(ktx::KTX::validate(ktxStorage));
        QVERIFY(ktxMemory->isValid());

        auto& outFile = TEST_IMAGE_KTX;
        if (!outFile.open()) {
            QFAIL("Unable to open file");
        }
        auto ktxSize = ktxStorage->size();
        outFile.resize(ktxSize);
        auto dest = outFile.map(0, ktxSize);
        memcpy(dest, ktxStorage->data(), ktxSize);
        outFile.unmap(dest);
        outFile.close();
    }


    {
        auto ktxStorage = std::make_shared<storage::FileStorage>(TEST_IMAGE_KTX.fileName());
        QVERIFY(ktx::KTX::validate(ktxStorage));
        auto ktxFile = ktx::KTX::create(ktxStorage);
        QVERIFY(ktxFile.get());
        QVERIFY(ktxFile->isValid());
        {
            const auto& memStorage = ktxMemory->getStorage();
            const auto& fileStorage = ktxFile->getStorage();
            QVERIFY(memStorage->size() == fileStorage->size());
            QVERIFY(memStorage->data() != fileStorage->data());
            QVERIFY(0 == memcmp(memStorage->data(), fileStorage->data(), memStorage->size()));
            QVERIFY(ktxFile->_images.size() == ktxMemory->_images.size());
            auto imageCount = ktxFile->_images.size();
            auto startMemory = ktxMemory->_storage->data();
            auto startFile = ktxFile->_storage->data();
            for (size_t i = 0; i < imageCount; ++i) {
                auto memImages = ktxMemory->_images[i];
                auto fileImages = ktxFile->_images[i];
                QVERIFY(memImages._padding == fileImages._padding);
                QVERIFY(memImages._numFaces == fileImages._numFaces);
                QVERIFY(memImages._imageSize == fileImages._imageSize);
                QVERIFY(memImages._faceSize == fileImages._faceSize);
                QVERIFY(memImages._faceBytes.size() == memImages._numFaces);
                QVERIFY(fileImages._faceBytes.size() == fileImages._numFaces);
                auto faceCount = fileImages._numFaces;
                for (uint32_t face = 0; face < faceCount; ++face) {
                    auto memFace = memImages._faceBytes[face];
                    auto memOffset = memFace - startMemory;
                    auto fileFace = fileImages._faceBytes[face];
                    auto fileOffset = fileFace - startFile;
                    QVERIFY(memOffset % 4 == 0);
                    QVERIFY(memOffset == fileOffset);
                }
            }
        }
    }
    testTexture->setKtxBacking(TEST_IMAGE_KTX.fileName().toStdString());
}

#if 0

static const QString TEST_FOLDER { "H:/ktx_cacheold" };
//static const QString TEST_FOLDER { "C:/Users/bdavis/Git/KTX/testimages" };

//static const QString EXTENSIONS { "4bbdf8f786470e4ab3e672d44b8e8df2.ktx" };
static const QString EXTENSIONS { "*.ktx" };

int mainTemp(int, char**) {
    qInstallMessageHandler(messageHandler);
    auto fileInfoList = QDir { TEST_FOLDER }.entryInfoList(QStringList  { EXTENSIONS });
    for (auto fileInfo : fileInfoList) {
        qDebug() << fileInfo.filePath();
        std::shared_ptr<storage::Storage> storage { new storage::FileStorage { fileInfo.filePath() } };

        if (!ktx::KTX::validate(storage)) {
            qDebug() << "KTX invalid";
        }

        auto ktxFile = ktx::KTX::create(storage);
        ktx::KTXDescriptor ktxDescriptor = ktxFile->toDescriptor();

        qDebug() << "Contains " << ktxDescriptor.keyValues.size() << " key value pairs";
        for (const auto& kv : ktxDescriptor.keyValues) {
            qDebug() << "\t" << kv._key.c_str();
        }

        auto offsetToMinMipKV = ktxDescriptor.getValueOffsetForKey(ktx::HIFI_MIN_POPULATED_MIP_KEY);
        if (offsetToMinMipKV) {
            auto data = storage->data() + ktx::KTX_HEADER_SIZE + offsetToMinMipKV;
            auto minMipLevelAvailable = *data;
            qDebug() << "\tMin mip available " << minMipLevelAvailable;
            assert(minMipLevelAvailable < ktxDescriptor.header.numberOfMipmapLevels);
        }
        auto storageSize = storage->size();
        for (const auto& faceImageDesc : ktxDescriptor.images) {
            //assert(0 == (faceImageDesc._faceSize % 4));
            for (const auto& faceOffset : faceImageDesc._faceOffsets) {
                assert(0 == (faceOffset % 4));
                auto faceEndOffset = faceOffset + faceImageDesc._faceSize;
                assert(faceEndOffset <= storageSize);
            }
        }

        for (const auto& faceImage : ktxFile->_images) {
            for (const ktx::Byte* faceBytes : faceImage._faceBytes) {
                assert(0 == (reinterpret_cast<size_t>(faceBytes) % 4));
            }
        }
    }
    return 0;
}
#endif
