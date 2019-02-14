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

#include "../FBXBaker.h"
#include "../OBJBaker.h"

QUrl getBakeableModelURL(const QUrl& url, bool shouldRebakeOriginals) {
    // Check if the file pointed to by this URL is a bakeable model, by comparing extensions
    auto modelFileName = url.fileName();

    bool isBakedModel = modelFileName.endsWith(BAKED_FBX_EXTENSION, Qt::CaseInsensitive);
    bool isBakeableFBX = modelFileName.endsWith(BAKEABLE_MODEL_FBX_EXTENSION, Qt::CaseInsensitive);
    bool isBakeableOBJ = modelFileName.endsWith(BAKEABLE_MODEL_OBJ_EXTENSION, Qt::CaseInsensitive);
    bool isBakeable = isBakeableFBX || isBakeableOBJ;

    if (isBakeable || (shouldRebakeOriginals && isBakedModel)) {
        if (isBakedModel) {
            // Grab a URL to the original, that we assume is stored a directory up, in the "original" folder
            // with just the fbx extension
            qDebug() << "Inferring original URL for baked model URL" << url;

            auto originalFileName = modelFileName;
            originalFileName.replace(".baked", "");
            qDebug() << "Original model URL must be present at" << url;

            return url.resolved("../original/" + originalFileName);
        } else {
            // Grab a clean version of the URL without a query or fragment
            return url.adjusted(QUrl::RemoveQuery | QUrl::RemoveFragment);
        }
    }

    qWarning() << "Unknown model type: " << modelFileName;
    return QUrl();
}

std::unique_ptr<ModelBaker> getModelBaker(const QUrl& bakeableModelURL, TextureBakerThreadGetter inputTextureThreadGetter, const QString& contentOutputPath) {
    auto filename = bakeableModelURL.fileName();

    // Output in a sub-folder with the name of the model, potentially suffixed by a number to make it unique
    auto baseName = filename.left(filename.lastIndexOf('.'));
    auto subDirName = "/" + baseName;
    int i = 1;
    while (QDir(contentOutputPath + subDirName).exists()) {
        subDirName = "/" + baseName + "-" + QString::number(i++);
    }

    QString bakedOutputDirectory = contentOutputPath + subDirName + "/baked";
    QString originalOutputDirectory = contentOutputPath + subDirName + "/original";

    std::unique_ptr<ModelBaker> baker;

    if (filename.endsWith(BAKEABLE_MODEL_FBX_EXTENSION, Qt::CaseInsensitive)) {
        baker = std::make_unique<FBXBaker>(bakeableModelURL, inputTextureThreadGetter, bakedOutputDirectory, originalOutputDirectory);
    } else if (filename.endsWith(BAKEABLE_MODEL_OBJ_EXTENSION, Qt::CaseInsensitive)) {
        baker = std::make_unique<OBJBaker>(bakeableModelURL, inputTextureThreadGetter, bakedOutputDirectory, originalOutputDirectory);
    } else {
        qDebug() << "Could not create ModelBaker for url" << bakeableModelURL;
    }

    if (baker) {
        QDir(contentOutputPath).mkpath(subDirName);
    }

    return baker;
}