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

#include <QtCore/QFile>
#include <QtNetwork/QNetworkReply>

#include <NetworkAccessManager.h>

#include "ModelBakingLoggingCategory.h"

#include "TextureBaker.h"

TextureBaker::TextureBaker(const QUrl& textureURL) :
    _textureURL(textureURL)
{
    
}

void TextureBaker::start() {

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

        // start the bake now that we have everything in place
        bake();
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
        connect(networkReply, &QNetworkReply::finished, this, &TextureBaker::handleTextureNetworkReply);
    }
}

void TextureBaker::handleTextureNetworkReply() {
    QNetworkReply* requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(model_baking) << "Downloaded texture at" << _textureURL;

        // store the original texture so it can be passed along for the bake
        _originalTexture = requestReply->readAll();

        // kickoff the texture bake now that everything is ready to go
        bake();
    } else {
        // add an error to our list stating that this texture could not be downloaded
        qCDebug(model_baking) << "Error downloading texture" << requestReply->errorString();

        emit finished();
    }
}

void TextureBaker::bake() {
    qCDebug(model_baking) << "Baking texture at" << _textureURL;

    // call image library to asynchronously bake this texture
    emit finished();
}
