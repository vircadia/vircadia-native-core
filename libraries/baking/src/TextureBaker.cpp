//
//  TextureBaker.cpp
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TextureBaker.h"

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkReply>

#include <image/TextureProcessing.h>
#include <ktx/KTX.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <SharedUtil.h>
#include <TextureMeta.h>

#include <OwningBuffer.h>

#include "ModelBakingLoggingCategory.h"

const QString BAKED_TEXTURE_KTX_EXT = ".ktx";
const QString BAKED_TEXTURE_BCN_SUFFIX = "_bcn.ktx";
const QString BAKED_META_TEXTURE_SUFFIX = ".texmeta.json";

bool TextureBaker::_compressionEnabled = true;

TextureBaker::TextureBaker(const QUrl& textureURL, image::TextureUsage::Type textureType,
                           const QDir& outputDirectory, const QString& baseFilename,
                           const QByteArray& textureContent) :
    _textureURL(textureURL),
    _originalTexture(textureContent),
    _textureType(textureType),
    _baseFilename(baseFilename),
    _outputDirectory(outputDirectory)
{
    if (baseFilename.isEmpty()) {
        // figure out the baked texture filename
        auto originalFilename = textureURL.fileName();
        _baseFilename = originalFilename.left(originalFilename.lastIndexOf('.'));
    }

    auto textureFilename = _textureURL.fileName();
    QString originalExtension;
    int extensionStart = textureFilename.indexOf(".");
    if (extensionStart != -1) {
        originalExtension = textureFilename.mid(extensionStart);
    }
    _originalCopyFilePath = _outputDirectory.absoluteFilePath(_baseFilename + originalExtension);
}

void TextureBaker::bake() {
    // once our texture is loaded, kick off a the processing
    connect(this, &TextureBaker::originalTextureLoaded, this, &TextureBaker::processTexture);

    if (_originalTexture.isEmpty()) {
        // first load the texture (either locally or remotely)
        loadTexture();
    } else {
        // we already have a texture passed to us, use that
        emit originalTextureLoaded();
    }
}

void TextureBaker::abort() {
    Baker::abort();

    // flip our atomic bool so any ongoing texture processing is stopped
    _abortProcessing.store(true);
}

void TextureBaker::loadTexture() {
    // check if the texture is local or first needs to be downloaded
    if (_textureURL.isLocalFile()) {
        // load up the local file
        QFile localTexture { _textureURL.toLocalFile() };

        if (!localTexture.open(QIODevice::ReadOnly)) {
            handleError("Unable to open texture " + _textureURL.toString());
            return;
        }

        _originalTexture = localTexture.readAll();

        emit originalTextureLoaded();
    } else {
        // remote file, kick off a download
        auto& networkAccessManager = NetworkAccessManager::getInstance();

        QNetworkRequest networkRequest;

        // setup the request to follow re-directs and always hit the network
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, NetworkingConstants::VIRCADIA_USER_AGENT);

        networkRequest.setUrl(_textureURL);

        qCDebug(model_baking) << "Downloading" << _textureURL;

        // kickoff the download, wait for slot to tell us it is done
        auto networkReply = networkAccessManager.get(networkRequest);
        connect(networkReply, &QNetworkReply::finished, this, &TextureBaker::handleTextureNetworkReply);
    }
}

void TextureBaker::handleTextureNetworkReply() {
    auto requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(model_baking) << "Downloaded texture" << _textureURL;

        // store the original texture so it can be passed along for the bake
        _originalTexture = requestReply->readAll();

        emit originalTextureLoaded();
    } else {
        // add an error to our list stating that this texture could not be downloaded
        handleError("Error downloading " + _textureURL.toString() + " - " + requestReply->errorString());
    }
}

void TextureBaker::processTexture() {
    // the baked textures need to have the source hash added for cache checks in Interface
    // so we add that to the processed texture before handling it off to be serialized
    QCryptographicHash hasher(QCryptographicHash::Md5);
    hasher.addData(_originalTexture);
    hasher.addData((const char*)&_textureType, sizeof(_textureType));
    auto hashData = hasher.result();
    std::string hash = hashData.toHex().toStdString();

    TextureMeta meta;

    QString originalCopyFilePath = _originalCopyFilePath.toString();

    // Copy the original file into the baked output directory if it doesn't exist yet
    {
        QFile file { originalCopyFilePath };
        if (!file.open(QIODevice::WriteOnly) || file.write(_originalTexture) == -1) {
            handleError("Could not write original texture for " + _textureURL.toString());
            return;
        }
        // IMPORTANT: _originalTexture is empty past this point
        _originalTexture.clear();
        _outputFiles.push_back(originalCopyFilePath);
        meta.original = _originalCopyFilePath.fileName();
    }

    // Load the copy of the original file from the baked output directory. New images will be created using the original as the source data.
    auto buffer = std::static_pointer_cast<QIODevice>(std::make_shared<QFile>(originalCopyFilePath));
    if (!buffer->open(QIODevice::ReadOnly)) {
        handleError("Could not open original file at " + originalCopyFilePath);
        return;
    }

    // Compressed KTX
    if (_compressionEnabled) {
        constexpr std::array<gpu::BackendTarget, 2> BACKEND_TARGETS {{
            gpu::BackendTarget::GL45,
            gpu::BackendTarget::GLES32
        }};
        for (auto target : BACKEND_TARGETS) {
            auto processedTextureAndSize = image::processImage(buffer, _textureURL.toString().toStdString(), image::ColorChannel::NONE,
                                                               ABSOLUTE_MAX_TEXTURE_NUM_PIXELS, _textureType, true,
                                                               target, _abortProcessing);
            if (!processedTextureAndSize.first) {
                handleError("Could not process texture " + _textureURL.toString());
                return;
            }
            processedTextureAndSize.first->setSourceHash(hash);

            if (shouldStop()) {
                return;
            }

            auto memKTX = gpu::Texture::serialize(*processedTextureAndSize.first, processedTextureAndSize.second);
            if (!memKTX) {
                handleError("Could not serialize " + _textureURL.toString() + " to KTX");
                return;
            }

            const char* name = khronos::gl::texture::toString(memKTX->_header.getGLInternaFormat());
            if (name == nullptr) {
                handleError("Could not determine internal format for compressed KTX: " + _textureURL.toString());
                return;
            }

            const char* data = reinterpret_cast<const char*>(memKTX->_storage->data());
            const size_t length = memKTX->_storage->size();

            auto fileName = _baseFilename + "_" + name + ".ktx";
            auto filePath = _outputDirectory.absoluteFilePath(fileName);
            QFile bakedTextureFile { filePath };
            if (!bakedTextureFile.open(QIODevice::WriteOnly) || bakedTextureFile.write(data, length) == -1) {
                handleError("Could not write baked texture for " + _textureURL.toString());
                return;
            }
            _outputFiles.push_back(filePath);
            meta.availableTextureTypes[memKTX->_header.getGLInternaFormat()] = fileName;
        }
    }

    // Uncompressed KTX
    if (_textureType == image::TextureUsage::Type::SKY_TEXTURE || _textureType == image::TextureUsage::Type::AMBIENT_TEXTURE) {
        buffer->reset();
        auto processedTextureAndSize = image::processImage(std::move(buffer), _textureURL.toString().toStdString(), image::ColorChannel::NONE,
                                                           ABSOLUTE_MAX_TEXTURE_NUM_PIXELS, _textureType, false, gpu::BackendTarget::GL45, _abortProcessing);
        if (!processedTextureAndSize.first) {
            handleError("Could not process texture " + _textureURL.toString());
            return;
        }
        processedTextureAndSize.first->setSourceHash(hash);

        if (shouldStop()) {
            return;
        }

        auto memKTX = gpu::Texture::serialize(*processedTextureAndSize.first, processedTextureAndSize.second);
        if (!memKTX) {
            handleError("Could not serialize " + _textureURL.toString() + " to KTX");
            return;
        }

        const char* data = reinterpret_cast<const char*>(memKTX->_storage->data());
        const size_t length = memKTX->_storage->size();

        auto fileName = _baseFilename + ".ktx";
        auto filePath = _outputDirectory.absoluteFilePath(fileName);
        QFile bakedTextureFile { filePath };
        if (!bakedTextureFile.open(QIODevice::WriteOnly) || bakedTextureFile.write(data, length) == -1) {
            handleError("Could not write baked texture for " + _textureURL.toString());
            return;
        }
        _outputFiles.push_back(filePath);
        meta.uncompressed = fileName;
    } else {
        buffer.reset();
    }

    {
        auto data = meta.serialize();
        _metaTextureFileName = _outputDirectory.absoluteFilePath(_baseFilename + BAKED_META_TEXTURE_SUFFIX);
        QFile file { _metaTextureFileName };
        if (!file.open(QIODevice::WriteOnly) || file.write(data) == -1) {
            handleError("Could not write meta texture for " + _textureURL.toString());
            return;
        } else {
            _outputFiles.push_back(_metaTextureFileName);
        }
    }

    qCDebug(model_baking) << "Baked texture" << _textureURL;
    setIsFinished(true);
}

void TextureBaker::setWasAborted(bool wasAborted) {
    Baker::setWasAborted(wasAborted);

    qCDebug(model_baking) << "Aborted baking" << _textureURL;
}
