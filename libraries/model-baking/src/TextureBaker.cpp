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

#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkReply>

#include <NetworkAccessManager.h>

#include "ModelBakingLoggingCategory.h"

#include "TextureBaker.h"

TextureBaker::TextureBaker(const QUrl& textureURL) :
    _textureURL(textureURL)
{
    
}

void TextureBaker::bake() {
    // first load the texture (either locally or remotely)
    loadTexture();

    qCDebug(model_baking) << "Baking texture at" << _textureURL;

    emit finished();
}

void TextureBaker::loadTexture() {
    // check if the texture is local or first needs to be downloaded
    if (_textureURL.isLocalFile()) {
        // load up the local file
        QFile localTexture { _textureURL.toLocalFile() };

        if (!localTexture.open(QIODevice::ReadOnly)) {
            qCWarning(model_baking) << "Unable to open local texture at" << _textureURL << "for baking";

            emit finished();
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
        qCDebug(model_baking) << "Error downloading texture" << requestReply->errorString();
    }
}
