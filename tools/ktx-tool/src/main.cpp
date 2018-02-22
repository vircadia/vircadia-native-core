#include <unordered_map>
#include <memory>
#include <cstdio>

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <QtGlobal>
#include <QtCore/QTime>
#include <QtCore/QTimer>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QLoggingCategory>

#include <QtCore/QResource>

#include <QtGui/QResizeEvent>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>

#include <gl/Config.h>

#include <gpu/Context.h>
#include <gpu/Batch.h>
#include <gpu/Stream.h>

#include <GLMHelpers.h>
#include <PathUtils.h>
#include <NumericalConstants.h>
#include <PerfStat.h>
#include <PathUtils.h>
#include <ViewFrustum.h>

#include <gpu/Pipeline.h>
#include <gpu/Context.h>
#include <gpu/Texture.h>

#include <ktx/KTX.h>




void stripKtxKeyValues(const std::string& sourceFile, const std::string& destFile) {
    auto sourceStorage = std::make_shared<storage::FileStorage>(sourceFile.c_str());
    auto ktx = ktx::KTX::create(sourceStorage);
    auto newStorageSize = ktx::KTX::evalStorageSize(ktx->_header, ktx->_images);
    storage::FileStorage::create(destFile.c_str(), newStorageSize, nullptr);
    storage::FileStorage destStorage(destFile.c_str());
    ktx::KTX::write(destStorage.mutableData(), newStorageSize, ktx->_header, ktx->_images);
}

#if DEV_BUILD
const QDir SOURCE_FOLDER{ PathUtils::projectRootPath() + "/interface/resources/meshes/mannequin" };
const QDir DEST_FOLDER{ PathUtils::projectRootPath() + "/interface/resources/meshes/mannequin/+gles" };
#else
const QDir SOURCE_FOLDER{ PathUtils::resourcesPath() + "/interface/resources/meshes/mannequin" };
const QDir DEST_FOLDER{ PathUtils::resourcesPath() + "/interface/resources/meshes/mannequin/+gles" };
#endif


void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
#ifdef Q_OS_WIN
    OutputDebugStringA(message.toStdString().c_str());
    OutputDebugStringA("\n");
#endif
    std::cout << message.toStdString() << std::endl;
}

static inline glm::uvec2 evalMipDimension(uint32_t mipLevel, const glm::uvec2& dims) {
    return glm::uvec2{
        std::max(dims.x >> mipLevel, 1U),
        std::max(dims.y >> mipLevel, 1U),
    };
}

void processKtxFile(const QFileInfo& inputFileInfo) {
    const QString inputFileName = inputFileInfo.absoluteFilePath();
    const QString compressedFileName = DEST_FOLDER.absoluteFilePath(inputFileInfo.fileName());
    const QString finalFilename = compressedFileName + ".new.ktx";
    if (QFileInfo(finalFilename).exists()) {
        return;
    }
    qDebug() << inputFileName;
    qDebug() << compressedFileName;
    auto uncomrpessedKtx = ktx::KTX::create(std::make_shared<storage::FileStorage>(compressedFileName));
    if (!uncomrpessedKtx) {
        throw std::runtime_error("Unable to load texture using hifi::ktx");
    }

    auto compressedKtx = ktx::KTX::create(std::make_shared<storage::FileStorage>(inputFileName));
    if (!compressedKtx) {
        throw std::runtime_error("Unable to load texture using hifi::ktx");
    }


    auto outputKtx = ktx::KTX::create(uncomrpessedKtx->getHeader(), uncomrpessedKtx->_images, compressedKtx->_keyValues);
    auto outputStorage = outputKtx->getStorage();

    storage::FileStorage::create(finalFilename, outputStorage->size(), outputStorage->data());
#if 0
    const auto& header = inputKtx->getHeader();


    detexTexture **input_textures;
    int nu_levels;
    if (!detexLoadTextureFileWithMipmaps(inputFileName.toStdString().c_str(), 32, &input_textures, &nu_levels)) {
        return;
        //throw std::runtime_error(detexGetErrorMessage());
    }

    switch (header.glInternalFormat) {
        case GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
            outputHeader.glFormat = GL_RGBA;
            outputHeader.glInternalFormat = GL_RGBA8;
            break;

        case GL_COMPRESSED_RED_GREEN_RGTC2_EXT:
            outputHeader.glFormat = GL_RG;
            outputHeader.glInternalFormat = GL_RG8;
            qWarning() << "Unsupported";
            return;
            break;

        case GL_COMPRESSED_RED_RGTC1_EXT:
            outputHeader.glFormat = GL_RED;
            outputHeader.glInternalFormat = GL_R8;
            qWarning() << "Unsupported";
            return;
            break;

        default:
            qWarning() << "Unsupported";
            return;
    }


    auto outputKtx = ktx::KTX::createBare(outputHeader, ktx->_keyValues);
    auto outputDescriptor = outputKtx->toDescriptor();
    auto outputStorage = outputKtx->getStorage();
    auto outputPointer = outputStorage->mutableData();
    auto outputSize = outputStorage->size();

    glm::uvec2 dimensions{ header.pixelWidth, header.pixelHeight };
    for (uint16_t mip = 0; mip < header.numberOfMipmapLevels; ++mip) {
        auto mipDimensions = evalMipDimension(mip, dimensions);

        for (uint8_t face = 0; face < header.numberOfFaces; ++face) {
            auto faceSize = outputDescriptor.getMipFaceTexelsSize(mip, face);
            auto faceOffset = outputDescriptor.getMipFaceTexelsOffset(mip, face);
            assert(faceOffset + faceSize <= outputSize);
            auto mipFace = ktx->getMipFaceTexelsData(mip, face);
            auto outputMipFace = outputKtx->getMipFaceTexelsData();
            if (header.glInternalFormat == GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT) {
                BlockDecompressImageDXT5(mipDimensions.x, mipDimensions.y, mipFace->data(), (unsigned long*)(outputPointer + faceOffset));
            } else if (header.glInternalFormat == GL_COMPRESSED_SRGB_S3TC_DXT1_EXT) {
                BlockDecompressImageDXT1(mipDimensions.x, mipDimensions.y, mipFace->data(), (unsigned long*)(outputPointer + faceOffset));
            } else {
                qWarning() << "Unsupported";
                return;
            }
        }
    }

    storage::FileStorage::create(outputFileName, outputStorage->size(), outputStorage->data());
#endif
    //if (gliTexture.empty()) {
    //    throw std::runtime_error("Unable to load texture using GLI");
    //}
    //gli::texture2d gliTextureConverted = gli::convert(gliTexture, gli::FORMAT_RGBA8_UNORM_PACK8);
}

void scanDir(const QDir& dir) {

    auto entries = dir.entryInfoList();
    for (const auto& entry : entries) {
        if (entry.isDir()) {
            scanDir(QDir(entry.absoluteFilePath()));
        } else {
            qDebug() << entry.absoluteFilePath();
        }
    }
}

int main(int argc, char** argv) {
    qInstallMessageHandler(messageHandler);
    {
        QDir destFolder(DEST_FOLDER);
        if (!destFolder.exists() && !destFolder.mkpath(".")) {
            throw std::runtime_error("failed to create output directory");
        }
        for (const auto ktxFile : SOURCE_FOLDER.entryInfoList(QStringList() << "*.ktx")) {
            processKtxFile(ktxFile);
        }
        qDebug() << "Done";
    }

    return 0;
}
