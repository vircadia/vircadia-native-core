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

#include "BakerCLI.h"

#include <QObject>
#include <QImageReader>
#include <QtCore/QDebug>
#include <QFile>

#include <unordered_map>

#include "OvenCLIApplication.h"
#include "ModelBakingLoggingCategory.h"
#include "baking/BakerLibrary.h"
#include "JSBaker.h"
#include "TextureBaker.h"

BakerCLI::BakerCLI(OvenCLIApplication* parent) : QObject(parent) {
    
}

void BakerCLI::bakeFile(QUrl inputUrl, const QString& outputPath, const QString& type) {

    // if the URL doesn't have a scheme, assume it is a local file
    if (inputUrl.scheme() != "http" && inputUrl.scheme() != "https" && inputUrl.scheme() != "ftp" && inputUrl.scheme() != "file") {
        inputUrl = QUrl::fromLocalFile(inputUrl.toString());
    }

    qDebug() << "Baking file type: " << type;

    static const QString MODEL_EXTENSION { "fbx" };
    static const QString SCRIPT_EXTENSION { "js" };

    // check what kind of baker we should be creating
    bool isModel = type == MODEL_EXTENSION;
    bool isScript = type == SCRIPT_EXTENSION;

    // If the type doesn't match the above, we assume we have a texture, and the type specified is the
    // texture usage type (albedo, cubemap, normals, etc.)
    auto url = inputUrl.toDisplayString();
    auto idx = url.lastIndexOf('.');
    auto extension = idx >= 0 ? url.mid(idx + 1).toLower() : "";
    bool isSupportedImage = QImageReader::supportedImageFormats().contains(extension.toLatin1());

    _outputPath = outputPath;

    // create our appropiate baker
    if (isModel) {
        QUrl bakeableModelURL = getBakeableModelURL(inputUrl, false);
        if (!bakeableModelURL.isEmpty()) {
            auto getWorkerThreadCallback = []() -> QThread* {
                return Oven::instance().getNextWorkerThread();
            };
            _baker = getModelBaker(bakeableModelURL, getWorkerThreadCallback, outputPath);
            if (_baker) {
                _baker->moveToThread(Oven::instance().getNextWorkerThread());
            }
        }
    } else if (isScript) {
        _baker = std::unique_ptr<Baker> { new JSBaker(inputUrl, outputPath) };
        _baker->moveToThread(Oven::instance().getNextWorkerThread());
    } else if (isSupportedImage) {
        static const std::unordered_map<QString, image::TextureUsage::Type> STRING_TO_TEXTURE_USAGE_TYPE_MAP {
            { "default", image::TextureUsage::DEFAULT_TEXTURE },
            { "strict", image::TextureUsage::STRICT_TEXTURE },
            { "albedo", image::TextureUsage::ALBEDO_TEXTURE },
            { "normal", image::TextureUsage::NORMAL_TEXTURE },
            { "bump", image::TextureUsage::BUMP_TEXTURE },
            { "specular", image::TextureUsage::SPECULAR_TEXTURE },
            { "metallic", image::TextureUsage::METALLIC_TEXTURE },
            { "roughness", image::TextureUsage::ROUGHNESS_TEXTURE },
            { "gloss", image::TextureUsage::GLOSS_TEXTURE },
            { "emissive", image::TextureUsage::EMISSIVE_TEXTURE },
            { "cube", image::TextureUsage::CUBE_TEXTURE },
            { "occlusion", image::TextureUsage::OCCLUSION_TEXTURE },
            { "scattering", image::TextureUsage::SCATTERING_TEXTURE },
            { "lightmap", image::TextureUsage::LIGHTMAP_TEXTURE },
        };

        auto it = STRING_TO_TEXTURE_USAGE_TYPE_MAP.find(type);
        if (it == STRING_TO_TEXTURE_USAGE_TYPE_MAP.end()) {
            qCDebug(model_baking) << "Unknown texture usage type:" << type;
            QCoreApplication::exit(OVEN_STATUS_CODE_FAIL);
        }
        _baker = std::unique_ptr<Baker> { new TextureBaker(inputUrl, it->second, outputPath) };
        _baker->moveToThread(Oven::instance().getNextWorkerThread());
    } else {
        qCDebug(model_baking) << "Failed to determine baker type for file" << inputUrl;
        QCoreApplication::exit(OVEN_STATUS_CODE_FAIL);
        return;
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
    QCoreApplication::exit(exitCode);
}
