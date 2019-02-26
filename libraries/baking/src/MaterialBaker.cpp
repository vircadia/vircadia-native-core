//
//  MaterialBaker.cpp
//  libraries/baking/src
//
//  Created by Sam Gondelman on 2/26/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MaterialBaker.h"

#include "QJsonObject"
#include "QJsonDocument"

#include "MaterialBakingLoggingCategory.h"

#include <SharedUtil.h>
#include <PathUtils.h>

MaterialBaker::MaterialBaker(const QString& materialData, bool isURL, const QString& bakedOutputDir) :
    _materialData(materialData),
    _isURL(isURL),
    _bakedOutputDir(bakedOutputDir)
{
}

void MaterialBaker::bake() {
    qDebug(material_baking) << "Material Baker" << _materialData << "bake starting";

    // once our script is loaded, kick off a the processing
    connect(this, &MaterialBaker::originalMaterialLoaded, this, &MaterialBaker::processMaterial);

    if (!_materialResource) {
        // first load the material (either locally or remotely)
        loadMaterial();
    } else {
        // we already have a material passed to us, use that
        if (_materialResource->isLoaded()) {
            emit originalMaterialLoaded();
        } else {
            connect(_materialResource.data(), &Resource::finished, this, &MaterialBaker::originalMaterialLoaded);
        }
    }
}

void MaterialBaker::loadMaterial() {
    if (!_isURL) {
        qCDebug(material_baking) << "Loading local material" << _materialData;

        _materialResource = NetworkMaterialResourcePointer(new NetworkMaterialResource());
        // TODO: add baseURL to allow these to reference relative files next to them
        _materialResource->parsedMaterials = NetworkMaterialResource::parseJSONMaterials(QJsonDocument::fromVariant(_materialData), QUrl());
    } else {
        qCDebug(material_baking) << "Downloading material" << _materialData;
        _materialResource = MaterialCache::instance().getMaterial(_materialData);
    }

    if (_materialResource) {
        if (_materialResource->isLoaded()) {
            emit originalMaterialLoaded();
        } else {
            connect(_materialResource.data(), &Resource::finished, this, &MaterialBaker::originalMaterialLoaded);
        }
    } else {
        handleError("Error loading " + _materialData);
    }
}

void MaterialBaker::processMaterial() {
    if (!_materialResource || _materialResource->parsedMaterials.networkMaterials.size() == 0) {
        handleError("Error processing " + _materialData);
    }

    _numTexturesToLoad = _materialResource->parsedMaterials.networkMaterials.size();
    _numTexturesLoaded = 0;

    for (auto networkMaterial : _materialResource->parsedMaterials.networkMaterials) {
        if (networkMaterial.second) {
            auto textureMaps = networkMaterial.second->getTextureMaps();
            for (auto textureMap : textureMaps) {
                if (textureMap.second && textureMap.second->getTextureSource()) {
                    auto texture = textureMap.second->getTextureSource();
                    graphics::Material::MapChannel mapChannel = textureMap.first;

                    qDebug() << "boop" << mapChannel << texture->getUrl();
                }
            }
        }
    }
}

void MaterialBaker::outputMaterial() {
    //if (_isURL) {
    //    auto fileName = _materialData;
    //    auto baseName = fileName.left(fileName.lastIndexOf('.'));
    //    auto bakedFilename = baseName + BAKED_MATERIAL_EXTENSION;

    //    _bakedMaterialData = _bakedOutputDir + "/" + bakedFilename;

    //    QFile bakedFile;
    //    bakedFile.setFileName(_bakedMaterialData);
    //    if (!bakedFile.open(QIODevice::WriteOnly)) {
    //        handleError("Error opening " + _bakedMaterialData + " for writing");
    //        return;
    //    }

    //    bakedFile.write(outputMaterial);

    //    // Export successful
    //    _outputFiles.push_back(_bakedMaterialData);
    //    qCDebug(material_baking) << "Exported" << _materialData << "to" << _bakedMaterialData;
    //}

    // emit signal to indicate the material baking is finished
    emit finished();
}
