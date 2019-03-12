//
//  BakerLibrary.cpp
//  libraries/baking/src/baking
//
//  Created by Sabrina Shanman on 2019/02/14.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "BakerLibrary.h"

#include "FSTBaker.h"
#include "../FBXBaker.h"
#include "../OBJBaker.h"

// Check if the file pointed to by this URL is a bakeable model, by comparing extensions
QUrl getBakeableModelURL(const QUrl& url) {
    static const std::vector<QString> extensionsToBake = {
        FST_EXTENSION,
        BAKED_FST_EXTENSION,
        FBX_EXTENSION,
        BAKED_FBX_EXTENSION,
        OBJ_EXTENSION,
        GLTF_EXTENSION
    };

    QUrl cleanURL = url.adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment);
    QString cleanURLString = cleanURL.fileName();
    for (auto& extension : extensionsToBake) {
        if (cleanURLString.endsWith(extension, Qt::CaseInsensitive)) {
            return cleanURL;
        }
    }

    qWarning() << "Unknown model type: " << url.fileName();
    return QUrl();
}

std::unique_ptr<ModelBaker> getModelBaker(const QUrl& bakeableModelURL, TextureBakerThreadGetter inputTextureThreadGetter, const QString& contentOutputPath) {
    auto filename = bakeableModelURL.fileName();

    // Output in a sub-folder with the name of the model, potentially suffixed by a number to make it unique
    auto baseName = filename.left(filename.lastIndexOf('.')).left(filename.lastIndexOf(".baked"));
    auto subDirName = "/" + baseName;
    int i = 1;
    while (QDir(contentOutputPath + subDirName).exists()) {
        subDirName = "/" + baseName + "-" + QString::number(i++);
    }

    QString bakedOutputDirectory = contentOutputPath + subDirName + "/baked";
    QString originalOutputDirectory = contentOutputPath + subDirName + "/original";

    return getModelBakerWithOutputDirectories(bakeableModelURL, inputTextureThreadGetter, bakedOutputDirectory, originalOutputDirectory);
}

std::unique_ptr<ModelBaker> getModelBakerWithOutputDirectories(const QUrl& bakeableModelURL, TextureBakerThreadGetter inputTextureThreadGetter, const QString& bakedOutputDirectory, const QString& originalOutputDirectory) {
    auto filename = bakeableModelURL.fileName();

    std::unique_ptr<ModelBaker> baker;
    if (filename.endsWith(FST_EXTENSION, Qt::CaseInsensitive)) {
        baker = std::make_unique<FSTBaker>(bakeableModelURL, inputTextureThreadGetter, bakedOutputDirectory, originalOutputDirectory, filename.endsWith(BAKED_FST_EXTENSION, Qt::CaseInsensitive));
    } else if (filename.endsWith(FBX_EXTENSION, Qt::CaseInsensitive)) {
        baker = std::make_unique<FBXBaker>(bakeableModelURL, inputTextureThreadGetter, bakedOutputDirectory, originalOutputDirectory, filename.endsWith(BAKED_FBX_EXTENSION, Qt::CaseInsensitive));
    } else if (filename.endsWith(OBJ_EXTENSION, Qt::CaseInsensitive)) {
        baker = std::make_unique<OBJBaker>(bakeableModelURL, inputTextureThreadGetter, bakedOutputDirectory, originalOutputDirectory);
    //} else if (filename.endsWith(GLTF_EXTENSION, Qt::CaseInsensitive)) {
        //baker = std::make_unique<GLTFBaker>(bakeableModelURL, inputTextureThreadGetter, bakedOutputDirectory, originalOutputDirectory);
    } else {
        qDebug() << "Could not create ModelBaker for url" << bakeableModelURL;
    }

    return baker;
}
