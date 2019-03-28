//
//  ModelBaker.cpp
//  libraries/baking/src
//
//  Created by Utkarsh Gautam on 9/29/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ModelBaker.h"

#include <PathUtils.h>
#include <NetworkAccessManager.h>

#include <DependencyManager.h>
#include <hfm/ModelFormatRegistry.h>
#include <FBXSerializer.h>

#include <model-baker/Baker.h>
#include <model-baker/PrepareJointsTask.h>

#include <FBXWriter.h>
#include <FSTReader.h>

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4267 )
#endif

#include <draco/mesh/triangle_soup_mesh_builder.h>
#include <draco/compression/encode.h>

#ifdef HIFI_DUMP_FBX
#include "FBXToJSON.h"
#endif

#ifdef _WIN32
#pragma warning( pop )
#endif

#include "baking/BakerLibrary.h"

#include <QJsonArray>

ModelBaker::ModelBaker(const QUrl& inputModelURL, const QUrl& destinationPath, const QString& bakedOutputDirectory, const QString& originalOutputDirectory, bool hasBeenBaked) :
    _modelURL(inputModelURL),
    _destinationPath(destinationPath),
    _bakedOutputDir(bakedOutputDirectory),
    _originalOutputDir(originalOutputDirectory),
    _hasBeenBaked(hasBeenBaked)
{
    auto bakedFilename = _modelURL.fileName();
    if (!hasBeenBaked) {
        bakedFilename = bakedFilename.left(bakedFilename.lastIndexOf('.'));
        bakedFilename += BAKED_FBX_EXTENSION;
    }
    _bakedModelURL = _bakedOutputDir + "/" + bakedFilename;
}

void ModelBaker::setOutputURLSuffix(const QUrl& outputURLSuffix) {
    _outputURLSuffix = outputURLSuffix;
}

void ModelBaker::setMappingURL(const QUrl& mappingURL) {
    _mappingURL = mappingURL;
}

void ModelBaker::setMapping(const hifi::VariantHash& mapping) {
    _mapping = mapping;
}

QUrl ModelBaker::getFullOutputMappingURL() const {
    QUrl appendedURL = _outputMappingURL;
    appendedURL.setFragment(_outputURLSuffix.fragment());
    appendedURL.setQuery(_outputURLSuffix.query());
    appendedURL.setUserInfo(_outputURLSuffix.userInfo());
    return appendedURL;
}

void ModelBaker::bake() {
    qDebug() << "ModelBaker" << _modelURL << "bake starting";

    // Setup the output folders for the results of this bake
    initializeOutputDirs();

    if (shouldStop()) {
        return;
    }

    connect(this, &ModelBaker::modelLoaded, this, &ModelBaker::bakeSourceCopy);

    // make a local copy of the model
    saveSourceModel();
}

void ModelBaker::initializeOutputDirs() {
    // Attempt to make the output folders
    // Warn if there is an output directory using the same name, unless we know a parent FST baker created them already

    if (QDir(_bakedOutputDir).exists()) {
        if (_mappingURL.isEmpty()) {
            qWarning() << "Output path" << _bakedOutputDir << "already exists. Continuing.";
        }
    } else {
        qCDebug(model_baking) << "Creating baked output folder" << _bakedOutputDir;
        if (!QDir().mkpath(_bakedOutputDir)) {
            handleError("Failed to create baked output folder " + _bakedOutputDir);
            return;
        }
    }

    QDir originalOutputDir { _originalOutputDir };
    if (originalOutputDir.exists()) {
        if (_mappingURL.isEmpty()) {
            qWarning() << "Output path" << _originalOutputDir << "already exists. Continuing.";
        }
    } else {
        qCDebug(model_baking) << "Creating original output folder" << _originalOutputDir;
        if (!QDir().mkpath(_originalOutputDir)) {
            handleError("Failed to create original output folder " + _originalOutputDir);
            return;
        }
    }

    if (originalOutputDir.isReadable()) {
        // The output directory is available. Use that to write/read the original model file
        _originalOutputModelPath = originalOutputDir.filePath(_modelURL.fileName());
    } else {
        handleError("Unable to write to original output folder " + _originalOutputDir);
    }
}

void ModelBaker::saveSourceModel() {
    // check if the FBX is local or first needs to be downloaded
    if (_modelURL.isLocalFile()) {
        // load up the local file
        QFile localModelURL { _modelURL.toLocalFile() };

        qDebug() << "Local file url: " << _modelURL << _modelURL.toString() << _modelURL.toLocalFile() << ", copying to: " << _originalOutputModelPath;

        if (!localModelURL.exists()) {
            //QMessageBox::warning(this, "Could not find " + _modelURL.toString(), "");
            handleError("Could not find " + _modelURL.toString());
            return;
        }

        localModelURL.copy(_originalOutputModelPath);

        // emit our signal to start the import of the model source copy
        emit modelLoaded();
    } else {
        // remote file, kick off a download
        auto& networkAccessManager = NetworkAccessManager::getInstance();

        QNetworkRequest networkRequest;

        // setup the request to follow re-directs and always hit the network
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);

        networkRequest.setUrl(_modelURL);

        qCDebug(model_baking) << "Downloading" << _modelURL;
        auto networkReply = networkAccessManager.get(networkRequest);

        connect(networkReply, &QNetworkReply::finished, this, &ModelBaker::handleModelNetworkReply);
    }
}

void ModelBaker::handleModelNetworkReply() {
    auto requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(model_baking) << "Downloaded" << _modelURL;

        // grab the contents of the reply and make a copy in the output folder
        QFile copyOfOriginal(_originalOutputModelPath);

        qDebug(model_baking) << "Writing copy of original model file to" << _originalOutputModelPath << copyOfOriginal.fileName();

        if (!copyOfOriginal.open(QIODevice::WriteOnly)) {
            // add an error to the error list for this model stating that a duplicate of the original model could not be made
            handleError("Could not create copy of " + _modelURL.toString() + " (Failed to open " + _originalOutputModelPath + ")");
            return;
        }
        if (copyOfOriginal.write(requestReply->readAll()) == -1) {
            handleError("Could not create copy of " + _modelURL.toString() + " (Failed to write)");
            return;
        }

        // close that file now that we are done writing to it
        copyOfOriginal.close();

        // emit our signal to start the import of the model source copy
        emit modelLoaded();
    } else {
        // add an error to our list stating that the model could not be downloaded
        handleError("Failed to download " + _modelURL.toString());
    }
}

void ModelBaker::bakeSourceCopy() {
    QFile modelFile(_originalOutputModelPath);
    if (!modelFile.open(QIODevice::ReadOnly)) {
        handleError("Error opening " + _originalOutputModelPath + " for reading");
        return;
    }
    hifi::ByteArray modelData = modelFile.readAll();

    std::vector<hifi::ByteArray> dracoMeshes;
    std::vector<std::vector<hifi::ByteArray>> dracoMaterialLists; // Material order for per-mesh material lookup used by dracoMeshes

    {
        auto serializer = DependencyManager::get<ModelFormatRegistry>()->getSerializerForMediaType(modelData, _modelURL, "");
        if (!serializer) {
            handleError("Could not recognize file type of model file " + _originalOutputModelPath);
            return;
        }
        hifi::VariantHash serializerMapping = _mapping;
        serializerMapping["combineParts"] = true; // set true so that OBJSerializer reads material info from material library
        serializerMapping["deduplicateIndices"] = true; // Draco compression also deduplicates, but we might as well shave it off to save on some earlier processing (currently FBXSerializer only)
        hfm::Model::Pointer loadedModel = serializer->read(modelData, serializerMapping, _modelURL);

        // Temporarily support copying the pre-parsed node from FBXSerializer, for better performance in FBXBaker
        // TODO: Pure HFM baking
        std::shared_ptr<FBXSerializer> fbxSerializer = std::dynamic_pointer_cast<FBXSerializer>(serializer);
        if (fbxSerializer) {
            qCDebug(model_baking) << "Parsing" << _modelURL;
            _rootNode = fbxSerializer->_rootNode;
        }

        baker::Baker baker(loadedModel, serializerMapping, _mappingURL);
        auto config = baker.getConfiguration();
        // Enable compressed draco mesh generation
        config->getJobConfig("BuildDracoMesh")->setEnabled(true);
        // Do not permit potentially lossy modification of joint data meant for runtime
        ((PrepareJointsConfig*)config->getJobConfig("PrepareJoints"))->passthrough = true;
        // The resources parsed from this job will not be used for now
        // TODO: Proper full baking of all materials for a model
        config->getJobConfig("ParseMaterialMapping")->setEnabled(false);
    
        // Begin hfm baking
        baker.run();

        _hfmModel = baker.getHFMModel();
        dracoMeshes = baker.getDracoMeshes();
        dracoMaterialLists = baker.getDracoMaterialLists();
    }

    // Do format-specific baking
    bakeProcessedSource(_hfmModel, dracoMeshes, dracoMaterialLists);

    if (shouldStop()) {
        return;
    }

    if (_hfmModel->materials.size() > 0) {
        _materialBaker = QSharedPointer<MaterialBaker>(
            new MaterialBaker(_modelURL.fileName(), true, _bakedOutputDir, _destinationPath),
            &MaterialBaker::deleteLater
        );
        _materialBaker->setMaterials(_hfmModel->materials, _modelURL.toString());
        connect(_materialBaker.data(), &MaterialBaker::finished, this, &ModelBaker::handleFinishedMaterialBaker);
        _materialBaker->bake();
    } else {
        outputBakedFST();
    }
}

void ModelBaker::handleFinishedMaterialBaker() {
    auto baker = qobject_cast<MaterialBaker*>(sender());

    if (baker) {
        if (!baker->hasErrors()) {
            auto bakedMaterialURL = baker->getBakedMaterialData();
            // this MaterialBaker is done and everything went according to plan
            qCDebug(model_baking) << "Adding baked material to FST mapping " << bakedMaterialURL;

            // First we add the materials in the model
            QJsonArray materialMapping;
            for (auto material : _hfmModel->materials) {
                QJsonObject json;
                json["mat::" + material.name] = bakedMaterialURL + "?" + material.name;
                materialMapping.push_back(json);
            }

            // The we add any existing mappings from the mapping
            if (_mapping.contains(MATERIAL_MAPPING_FIELD)) {
                QByteArray materialMapValue = _mapping[MATERIAL_MAPPING_FIELD].toByteArray();
                QJsonObject oldMaterialMapping = QJsonDocument::fromJson(materialMapValue).object();
                for (auto key : oldMaterialMapping.keys()) {
                    QJsonObject json;
                    json[key] = oldMaterialMapping[key];
                    materialMapping.push_back(json);
                }
            }

            _mapping[MATERIAL_MAPPING_FIELD] = QJsonDocument(materialMapping).toJson(QJsonDocument::Compact);
        } else {
            // this material failed to bake - this doesn't fail the entire bake but we need to add the errors from
            // the material to our warnings
            _warningList << baker->getWarnings();
        }
    } else {
        handleWarning("Failed to bake the materials for model with URL " + _modelURL.toString());
    }

    outputBakedFST();
}

void ModelBaker::outputBakedFST() {
    // Output FST file, copying over input mappings if available
    QString outputFSTFilename = !_mappingURL.isEmpty() ? _mappingURL.fileName() : _modelURL.fileName();
    auto extensionStart = outputFSTFilename.indexOf(".");
    if (extensionStart != -1) {
        outputFSTFilename.resize(extensionStart);
    }
    outputFSTFilename += ".baked.fst";
    QString outputFSTURL = _bakedOutputDir + "/" + outputFSTFilename;

    auto outputMapping = _mapping;
    outputMapping[FST_VERSION_FIELD] = FST_VERSION;
    outputMapping[FILENAME_FIELD] = _bakedModelURL.fileName();
    hifi::ByteArray fstOut = FSTReader::writeMapping(outputMapping);

    QFile fstOutputFile { outputFSTURL };
    if (!fstOutputFile.open(QIODevice::WriteOnly)) {
        handleError("Failed to open file '" + outputFSTURL + "' for writing");
        return;
    }
    if (fstOutputFile.write(fstOut) == -1) {
        handleError("Failed to write to file '" + outputFSTURL + "'");
        return;
    }
    _outputFiles.push_back(outputFSTURL);
    _outputMappingURL = outputFSTURL;

    exportScene();
    qCDebug(model_baking) << "Finished baking, emitting finished" << _modelURL;
    emit finished();
}

void ModelBaker::abort() {
    Baker::abort();

    if (_materialBaker) {
        _materialBaker->abort();
    }
}

bool ModelBaker::buildDracoMeshNode(FBXNode& dracoMeshNode, const QByteArray& dracoMeshBytes, const std::vector<hifi::ByteArray>& dracoMaterialList) {
    if (dracoMeshBytes.isEmpty()) {
        handleError("Failed to finalize the baking of a draco Geometry node");
        return false;
    }

    FBXNode dracoNode;
    dracoNode.name = "DracoMesh";
    dracoNode.properties.append(QVariant::fromValue(dracoMeshBytes));
    // Additional draco mesh node information
    {
        FBXNode fbxVersionNode;
        fbxVersionNode.name = "FBXDracoMeshVersion";
        fbxVersionNode.properties.append(FBX_DRACO_MESH_VERSION);
        dracoNode.children.append(fbxVersionNode);

        FBXNode dracoVersionNode;
        dracoVersionNode.name = "DracoMeshVersion";
        dracoVersionNode.properties.append(DRACO_MESH_VERSION);
        dracoNode.children.append(dracoVersionNode);

        FBXNode materialListNode;
        materialListNode.name = "MaterialList";
        for (const hifi::ByteArray& materialID : dracoMaterialList) {
            materialListNode.properties.append(materialID);
        }
        dracoNode.children.append(materialListNode);
    }
    
    dracoMeshNode = dracoNode;
    return true;
}

void ModelBaker::setWasAborted(bool wasAborted) {
    if (wasAborted != _wasAborted.load()) {
        Baker::setWasAborted(wasAborted);

        if (wasAborted) {
            qCDebug(model_baking) << "Aborted baking" << _modelURL;
        }
    }
}

void ModelBaker::exportScene() {
    auto fbxData = FBXWriter::encodeFBX(_rootNode);

    QString bakedModelURL = _bakedModelURL.toString();
    QFile bakedFile(bakedModelURL);

    if (!bakedFile.open(QIODevice::WriteOnly)) {
        handleError("Error opening " + bakedModelURL + " for writing");
        return;
    }

    bakedFile.write(fbxData);

    _outputFiles.push_back(bakedModelURL);

#ifdef HIFI_DUMP_FBX
    {
        FBXToJSON fbxToJSON;
        fbxToJSON << _rootNode;
        QFileInfo modelFile(_bakedModelURL.toString());
        QString outFilename(modelFile.dir().absolutePath() + "/" + modelFile.completeBaseName() + "_FBX.json");
        QFile jsonFile(outFilename);
        if (jsonFile.open(QIODevice::WriteOnly)) {
            jsonFile.write(fbxToJSON.str().c_str(), fbxToJSON.str().length());
            jsonFile.close();
        }
    }
#endif

    qCDebug(model_baking) << "Exported" << _modelURL << "with re-written paths to" << bakedModelURL;
}
