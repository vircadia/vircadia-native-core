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
#include <QFile>

#include "ModelBakingLoggingCategory.h"
#include "Oven.h"
#include "BakerCLI.h"
#include "FBXBaker.h"
#include "TextureBaker.h"

BakerCLI::BakerCLI(Oven* parent) : QObject(parent) {
}

void BakerCLI::bakeFile(QUrl inputUrl, const QString& outputPath, const QString& type) {

    // if the URL doesn't have a scheme, assume it is a local file
    if (inputUrl.scheme() != "http" && inputUrl.scheme() != "https" && inputUrl.scheme() != "ftp") {
        inputUrl.setScheme("file");
    }

    qDebug() << "Baking file type: " << type;

    static const QString MODEL_EXTENSION { "fbx" };

    QString extension = type;

    if (extension.isNull()) {
        auto url = inputUrl.toDisplayString();
        extension = url.mid(url.lastIndexOf('.'));
    }

    // check what kind of baker we should be creating
    bool isFBX = extension == MODEL_EXTENSION;

    bool isSupportedImage = QImageReader::supportedImageFormats().contains(extension.toLatin1());

    _outputPath = outputPath;

    // create our appropiate baker
    if (isFBX) {
        _baker = std::unique_ptr<Baker> { new FBXBaker(inputUrl, []() -> QThread* { return qApp->getNextWorkerThread(); }, outputPath) };
        _baker->moveToThread(qApp->getNextWorkerThread());
    } else if (isSupportedImage) {
        _baker = std::unique_ptr<Baker> { new TextureBaker(inputUrl, image::TextureUsage::CUBE_TEXTURE, outputPath) };
        _baker->moveToThread(qApp->getNextWorkerThread());
    } else {
        qCDebug(model_baking) << "Failed to determine baker type for file" << inputUrl;
        QApplication::exit(OVEN_STATUS_CODE_FAIL);
    }

    // invoke the bake method on the baker thread
    QMetaObject::invokeMethod(_baker.get(), "bake");

    // make sure we hear about the results of this baker when it is done
    connect(_baker.get(), &Baker::finished, this, &BakerCLI::handleFinishedBaker);
}

void BakerCLI::handleFinishedBaker() {
    qCDebug(model_baking) << "Finished baking file.";
    int exitCode = OVEN_STATUS_CODE_SUCCESS;
    // Do we need this?
    if (_baker->wasAborted()) {
        exitCode = OVEN_STATUS_CODE_ABORT;
    } else if (_baker->hasErrors()) {
        exitCode = OVEN_STATUS_CODE_FAIL;
        QFile errorFile { _outputPath.absoluteFilePath(OVEN_ERROR_FILENAME) };
        if (errorFile.open(QFile::WriteOnly)) {
            errorFile.write(_baker->getErrors().join('\n').toUtf8());
            errorFile.close();
        }
    }
    QApplication::exit(exitCode);
}
