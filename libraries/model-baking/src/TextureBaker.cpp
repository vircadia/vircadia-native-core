//
//  TextureBaker.cpp
//  libraries/model-baker/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkReply>

#include <image/Image.h>
#include <ktx/KTX.h>
#include <NetworkAccessManager.h>

#include "ModelBakingLoggingCategory.h"

#include "TextureBaker.h"

const QString BAKED_TEXTURE_EXT = ".ktx";

TextureBaker::TextureBaker(const QUrl& textureURL, gpu::TextureType textureType, const QString& destinationFilePath) :
    _textureURL(textureURL),
    _textureType(textureType),
    _destinationFilePath(destinationFilePath)
{

}

void TextureBaker::bake() {
    // first load the texture (either locally or remotely)
    loadTexture();

    if (hasErrors()) {
        return;
    }

    qCDebug(model_baking) << "Baking texture at" << _textureURL;

    processTexture();

    if (hasErrors()) {
        return;
    }

    qCDebug(model_baking) << "Baked texture at" << _textureURL;

    emit finished();
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
    } else {
        // remote file, kick off a download
        auto& networkAccessManager = NetworkAccessManager::getInstance();

        QNetworkRequest networkRequest;

        // setup the request to follow re-directs and always hit the network
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);

        networkRequest.setUrl(_textureURL);

        qCDebug(model_baking) << "Downloading" << _textureURL;

        auto networkReply = networkAccessManager.get(networkRequest);

        // use an event loop to process events while we wait for the network reply
        QEventLoop eventLoop;
        connect(networkReply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();

        handleTextureNetworkReply(networkReply);
    }
}

void TextureBaker::handleTextureNetworkReply(QNetworkReply* requestReply) {
    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(model_baking) << "Downloaded texture at" << _textureURL;

        // store the original texture so it can be passed along for the bake
        _originalTexture = requestReply->readAll();
    } else {
        // add an error to our list stating that this texture could not be downloaded
        handleError("Error downloading " + _textureURL.toString() + " - " + requestReply->errorString());
    }
}

void TextureBaker::processTexture() {
    auto processedTexture = image::processImage(_originalTexture, _textureURL.toString().toStdString(),
                                                ABSOLUTE_MAX_TEXTURE_NUM_PIXELS, _textureType);

    if (!processedTexture) {
        handleError("Could not process texture " + _textureURL.toString());
        return;
    }

    auto memKTX = gpu::Texture::serialize(*processedTexture);

    if (!memKTX) {
        handleError("Could not serialize " + _textureURL.toString() + " to KTX");
        return;
    }

    const char* data = reinterpret_cast<const char*>(memKTX->_storage->data());
    const size_t length = memKTX->_storage->size();

    // attempt to write the baked texture to the destination file path
    QFile bakedTextureFile { _destinationFilePath };

    if (!bakedTextureFile.open(QIODevice::WriteOnly) || bakedTextureFile.write(data, length) == -1) {
        handleError("Could not write baked texture for " + _textureURL.toString());
    }
}
