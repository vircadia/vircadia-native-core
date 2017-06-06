//
//  BakerCLI.cpp
//  tools/oven/src
//
//  Created by Robbie Uvanni on 6/6/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QImageReader>
#include <QtCore/QDebug>
#include "ModelBakingLoggingCategory.h"

#include "Oven.h"
#include "BakerCLI.h"
#include "FBXBaker.h"
#include "TextureBaker.h"

void BakerCLI::bakeFile(const QString inputFilename, const QString outputFilename) {
    QUrl inputUrl(inputFilename);

    // if the URL doesn't have a scheme, assume it is a local file
    if (inputUrl.scheme() != "http" && inputUrl.scheme() != "https" && inputUrl.scheme() != "ftp") {
        inputUrl.setScheme("file");
    }

    static const QString MODEL_EXTENSION { ".fbx" };

    // check what kind of baker we should be creating
    bool isFBX = inputFilename.endsWith(MODEL_EXTENSION, Qt::CaseInsensitive);
    bool isSupportedImage = false;

    for (QByteArray format : QImageReader::supportedImageFormats()) {
        isSupportedImage |= inputFilename.endsWith(format, Qt::CaseInsensitive);
    }

    // create our appropiate baker
    Baker* baker;

    if (isFBX) {
        baker = new FBXBaker(inputUrl, outputFilename, []() -> QThread* {
            return qApp->getNextWorkerThread();
        });
        baker->moveToThread(qApp->getFBXBakerThread());
    } else if (isSupportedImage) {
        baker = new TextureBaker(inputUrl, image::TextureUsage::CUBE_TEXTURE, outputFilename);
        baker->moveToThread(qApp->getNextWorkerThread());
    } else {
        qCDebug(model_baking) << "Failed to determine baker type for file" << inputUrl;
        return;
    }

    // invoke the bake method on the baker thread
    QMetaObject::invokeMethod(baker, "bake");

    // make sure we hear about the results of this baker when it is done
    connect(baker, &Baker::finished, this, &BakerCLI::handleFinishedBaker);
}

void BakerCLI::handleFinishedBaker() {
    qCDebug(model_baking) << "Finished baking file.";
    sender()->deleteLater();
    QApplication::quit();
}