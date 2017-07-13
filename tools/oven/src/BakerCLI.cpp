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

#include <QObject>
#include <QImageReader>
#include <QtCore/QDebug>

#include "ModelBakingLoggingCategory.h"
#include "Oven.h"
#include "BakerCLI.h"
#include "FBXBaker.h"
#include "TextureBaker.h"

BakerCLI::BakerCLI(Oven* parent) : QObject(parent) {
}

void BakerCLI::bakeFile(QUrl inputUrl, const QString outputPath) {

    // if the URL doesn't have a scheme, assume it is a local file
    if (inputUrl.scheme() != "http" && inputUrl.scheme() != "https" && inputUrl.scheme() != "ftp") {
        inputUrl.setScheme("file");
    }

    static const QString MODEL_EXTENSION { ".fbx" };

    // check what kind of baker we should be creating
    bool isFBX = inputUrl.toDisplayString().endsWith(MODEL_EXTENSION, Qt::CaseInsensitive);
    bool isSupportedImage = false;

    for (QByteArray format : QImageReader::supportedImageFormats()) {
        isSupportedImage |= inputUrl.toDisplayString().endsWith(format, Qt::CaseInsensitive);
    }

    // create our appropiate baker
    if (isFBX) {
        _baker = std::unique_ptr<Baker> { new FBXBaker(inputUrl, outputPath, []() -> QThread* { return qApp->getNextWorkerThread(); }) };
        _baker->moveToThread(qApp->getFBXBakerThread());
    } else if (isSupportedImage) {
        _baker = std::unique_ptr<Baker> { new TextureBaker(inputUrl, image::TextureUsage::CUBE_TEXTURE, outputPath) };
        _baker->moveToThread(qApp->getNextWorkerThread());
    } else {
        qCDebug(model_baking) << "Failed to determine baker type for file" << inputUrl;
        QApplication::exit(1);
    }

    // invoke the bake method on the baker thread
    QMetaObject::invokeMethod(_baker.get(), "bake");

    // make sure we hear about the results of this baker when it is done
    connect(_baker.get(), &Baker::finished, this, &BakerCLI::handleFinishedBaker);
}

void BakerCLI::handleFinishedBaker() {
    qCDebug(model_baking) << "Finished baking file.";
    QApplication::exit(_baker.get()->hasErrors());
}