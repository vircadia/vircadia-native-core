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

ModelBaker::ModelBaker(const QUrl& inputModelURL, TextureBakerThreadGetter inputTextureThreadGetter,
                       const QString& bakedOutputDirectory, const QString& originalOutputDirectory, bool hasBeenBaked) :
    _modelURL(inputModelURL),
    _bakedOutputDir(bakedOutputDirectory),
    _originalOutputDir(originalOutputDirectory),
    _textureThreadGetter(inputTextureThreadGetter),
    _hasBeenBaked(hasBeenBaked)
{
    auto tempDir = PathUtils::generateTemporaryDir();

    if (tempDir.isEmpty()) {
        handleError("Failed to create a temporary directory.");
        return;
    }

    _modelTempDir = tempDir;

    _originalModelFilePath = _modelTempDir.filePath(_modelURL.fileName());
    qDebug() << "Made temporary dir " << _modelTempDir;
    qDebug() << "Origin file path: " << _originalModelFilePath;

    {
        auto bakedFilename = _modelURL.fileName();
        if (!hasBeenBaked) {
            bakedFilename = bakedFilename.left(bakedFilename.lastIndexOf('.'));
            bakedFilename += BAKED_FBX_EXTENSION;
        }
        _bakedModelURL = _bakedOutputDir + "/" + bakedFilename;
    }
}

ModelBaker::~ModelBaker() {
    if (_modelTempDir.exists()) {
        if (!_modelTempDir.remove(_originalModelFilePath)) {
            qCWarning(model_baking) << "Failed to remove temporary copy of model file:" << _originalModelFilePath;
        }
        if (!_modelTempDir.rmdir(".")) {
            qCWarning(model_baking) << "Failed to remove temporary directory:" << _modelTempDir;
        }
    }
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

    if (QDir(_originalOutputDir).exists()) {
        if (_mappingURL.isEmpty()) {
            qWarning() << "Output path" << _originalOutputDir << "already exists. Continuing.";
        }
    } else {
        qCDebug(model_baking) << "Creating original output folder" << _originalOutputDir;
        if (!QDir().mkpath(_originalOutputDir)) {
            handleError("Failed to create original output folder " + _originalOutputDir);
        }
    }
}

void ModelBaker::saveSourceModel() {
    // check if the FBX is local or first needs to be downloaded
    if (_modelURL.isLocalFile()) {
        // load up the local file
        QFile localModelURL { _modelURL.toLocalFile() };

        qDebug() << "Local file url: " << _modelURL << _modelURL.toString() << _modelURL.toLocalFile() << ", copying to: " << _originalModelFilePath;

        if (!localModelURL.exists()) {
            //QMessageBox::warning(this, "Could not find " + _modelURL.toString(), "");
            handleError("Could not find " + _modelURL.toString());
            return;
        }

        // make a copy in the output folder
        if (!_originalOutputDir.isEmpty()) {
            qDebug() << "Copying to: " << _originalOutputDir << "/" << _modelURL.fileName();
            localModelURL.copy(_originalOutputDir + "/" + _modelURL.fileName());
        }

        localModelURL.copy(_originalModelFilePath);

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
        QFile copyOfOriginal(_originalModelFilePath);

        qDebug(model_baking) << "Writing copy of original model file to" << _originalModelFilePath << copyOfOriginal.fileName();

        if (!copyOfOriginal.open(QIODevice::WriteOnly)) {
            // add an error to the error list for this model stating that a duplicate of the original model could not be made
            handleError("Could not create copy of " + _modelURL.toString() + " (Failed to open " + _originalModelFilePath + ")");
            return;
        }
        if (copyOfOriginal.write(requestReply->readAll()) == -1) {
            handleError("Could not create copy of " + _modelURL.toString() + " (Failed to write)");
            return;
        }

        // close that file now that we are done writing to it
        copyOfOriginal.close();

        if (!_originalOutputDir.isEmpty()) {
            copyOfOriginal.copy(_originalOutputDir + "/" + _modelURL.fileName());
        }

        // emit our signal to start the import of the model source copy
        emit modelLoaded();
    } else {
        // add an error to our list stating that the model could not be downloaded
        handleError("Failed to download " + _modelURL.toString());
    }
}

void ModelBaker::bakeSourceCopy() {
    QFile modelFile(_originalModelFilePath);
    if (!modelFile.open(QIODevice::ReadOnly)) {
        handleError("Error opening " + _originalModelFilePath + " for reading");
        return;
    }
    hifi::ByteArray modelData = modelFile.readAll();

    hfm::Model::Pointer bakedModel;
    std::vector<hifi::ByteArray> dracoMeshes;
    std::vector<std::vector<hifi::ByteArray>> dracoMaterialLists; // Material order for per-mesh material lookup used by dracoMeshes

    {
        auto serializer = DependencyManager::get<ModelFormatRegistry>()->getSerializerForMediaType(modelData, _modelURL, "");
        if (!serializer) {
            handleError("Could not recognize file type of model file " + _originalModelFilePath);
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

        bakedModel = baker.getHFMModel();
        dracoMeshes = baker.getDracoMeshes();
        dracoMaterialLists = baker.getDracoMaterialLists();
    }

    // Populate _textureContentMap with path to content mappings, for quick lookup by URL
    for (auto materialIt = bakedModel->materials.cbegin(); materialIt != bakedModel->materials.cend(); materialIt++) {
        static const auto addTexture = [](QHash<hifi::ByteArray, hifi::ByteArray>& textureContentMap, const hfm::Texture& texture) {
            if (!textureContentMap.contains(texture.filename)) {
                // Content may be empty, unless the data is inlined
                textureContentMap[texture.filename] = texture.content;
            }
        };
        const hfm::Material& material = *materialIt;
        addTexture(_textureContentMap, material.normalTexture);
        addTexture(_textureContentMap, material.albedoTexture);
        addTexture(_textureContentMap, material.opacityTexture);
        addTexture(_textureContentMap, material.glossTexture);
        addTexture(_textureContentMap, material.roughnessTexture);
        addTexture(_textureContentMap, material.specularTexture);
        addTexture(_textureContentMap, material.metallicTexture);
        addTexture(_textureContentMap, material.emissiveTexture);
        addTexture(_textureContentMap, material.occlusionTexture);
        addTexture(_textureContentMap, material.scatteringTexture);
        addTexture(_textureContentMap, material.lightmapTexture);
    }

    // Do format-specific baking
    bakeProcessedSource(bakedModel, dracoMeshes, dracoMaterialLists);

    if (shouldStop()) {
        return;
    }

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
    // All textures will be found in the same directory as the model
    outputMapping[TEXDIR_FIELD] = ".";
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

    // check if we're already done with textures (in case we had none to re-write)
    checkIfTexturesFinished();
}

void ModelBaker::abort() {
    Baker::abort();

    // tell our underlying TextureBaker instances to abort
    // the ModelBaker will wait until all are aborted before emitting its own abort signal
    for (auto& textureBaker : _bakingTextures) {
        textureBaker->abort();
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

QString ModelBaker::compressTexture(QString modelTextureFileName, image::TextureUsage::Type textureType) {

    QFileInfo modelTextureFileInfo { modelTextureFileName.replace("\\", "/") };
    
    if (modelTextureFileInfo.suffix().toLower() == BAKED_TEXTURE_KTX_EXT.mid(1)) {
        // re-baking a model that already references baked textures
        // this is an error - return from here
        handleError("Cannot re-bake a file that already references compressed textures");
        return QString::null;
    }

    if (!image::getSupportedFormats().contains(modelTextureFileInfo.suffix())) {
        // this is a texture format we don't bake, skip it
        handleWarning(modelTextureFileName + " is not a bakeable texture format");
        return QString::null;
    }

    // make sure this texture points to something and isn't one we've already re-mapped
    QString textureChild { QString::null };
    if (!modelTextureFileInfo.filePath().isEmpty()) {
        // check if this was an embedded texture that we already have in-memory content for
        QByteArray textureContent;
        
        // figure out the URL to this texture, embedded or external
        if (!modelTextureFileInfo.filePath().isEmpty()) {
            textureContent = _textureContentMap.value(modelTextureFileName.toLocal8Bit());
        }
        auto urlToTexture = getTextureURL(modelTextureFileInfo, !textureContent.isNull());

        QString baseTextureFileName;
        if (_remappedTexturePaths.contains(urlToTexture)) {
            baseTextureFileName = _remappedTexturePaths[urlToTexture];
        } else {
            // construct the new baked texture file name and file path
            // ensuring that the baked texture will have a unique name
            // even if there was another texture with the same name at a different path
            baseTextureFileName = _textureFileNamer.createBaseTextureFileName(modelTextureFileInfo, textureType);
            _remappedTexturePaths[urlToTexture] = baseTextureFileName;
        }

        qCDebug(model_baking).noquote() << "Re-mapping" << modelTextureFileName
            << "to" << baseTextureFileName;

        QString bakedTextureFilePath {
            _bakedOutputDir + "/" + baseTextureFileName + BAKED_META_TEXTURE_SUFFIX
        };

        textureChild = baseTextureFileName + BAKED_META_TEXTURE_SUFFIX;

        if (!_bakingTextures.contains(urlToTexture)) {
            _outputFiles.push_back(bakedTextureFilePath);

            // bake this texture asynchronously
            bakeTexture(urlToTexture, textureType, _bakedOutputDir, baseTextureFileName, textureContent);
        }
    }
   
    return textureChild;
}

void ModelBaker::bakeTexture(const QUrl& textureURL, image::TextureUsage::Type textureType,
                             const QDir& outputDir, const QString& bakedFilename, const QByteArray& textureContent) {
    
    // start a bake for this texture and add it to our list to keep track of
    QSharedPointer<TextureBaker> bakingTexture{
        new TextureBaker(textureURL, textureType, outputDir, "../", bakedFilename, textureContent),
        &TextureBaker::deleteLater
    };
    
    // make sure we hear when the baking texture is done or aborted
    connect(bakingTexture.data(), &Baker::finished, this, &ModelBaker::handleBakedTexture);
    connect(bakingTexture.data(), &TextureBaker::aborted, this, &ModelBaker::handleAbortedTexture);

    // keep a shared pointer to the baking texture
    _bakingTextures.insert(textureURL, bakingTexture);

    // start baking the texture on one of our available worker threads
    bakingTexture->moveToThread(_textureThreadGetter());
    QMetaObject::invokeMethod(bakingTexture.data(), "bake");
}

void ModelBaker::handleBakedTexture() {
    TextureBaker* bakedTexture = qobject_cast<TextureBaker*>(sender());
    qDebug() << "Handling baked texture" << bakedTexture->getTextureURL();

    // make sure we haven't already run into errors, and that this is a valid texture
    if (bakedTexture) {
        if (!shouldStop()) {
            if (!bakedTexture->hasErrors()) {
                if (!_originalOutputDir.isEmpty()) {
                    // we've been asked to make copies of the originals, so we need to make copies of this if it is a linked texture

                    // use the path to the texture being baked to determine if this was an embedded or a linked texture

                    // it is embeddded if the texure being baked was inside a folder with the name of the model
                    // since that is the fake URL we provide when baking external textures

                    if (!_modelURL.isParentOf(bakedTexture->getTextureURL())) {
                        // for linked textures we want to save a copy of original texture beside the original model

                        qCDebug(model_baking) << "Saving original texture for" << bakedTexture->getTextureURL();

                        // check if we have a relative path to use for the texture
                        auto relativeTexturePath = texturePathRelativeToModel(_modelURL, bakedTexture->getTextureURL());

                        QFile originalTextureFile{
                            _originalOutputDir + "/" + relativeTexturePath + bakedTexture->getTextureURL().fileName()
                        };

                        if (relativeTexturePath.length() > 0) {
                            // make the folders needed by the relative path
                        }

                        if (originalTextureFile.open(QIODevice::WriteOnly) && originalTextureFile.write(bakedTexture->getOriginalTexture()) != -1) {
                            qCDebug(model_baking) << "Saved original texture file" << originalTextureFile.fileName()
                                << "for" << _modelURL;
                        } else {
                            handleError("Could not save original external texture " + originalTextureFile.fileName()
                                        + " for " + _modelURL.toString());
                            return;
                        }
                    }
                }


                // now that this texture has been baked and handled, we can remove that TextureBaker from our hash
                _bakingTextures.remove(bakedTexture->getTextureURL());

                checkIfTexturesFinished();
            } else {
                // there was an error baking this texture - add it to our list of errors
                _errorList.append(bakedTexture->getErrors());

                // we don't emit finished yet so that the other textures can finish baking first
                _pendingErrorEmission = true;

                // now that this texture has been baked, even though it failed, we can remove that TextureBaker from our list
                _bakingTextures.remove(bakedTexture->getTextureURL());

                // abort any other ongoing texture bakes since we know we'll end up failing
                for (auto& bakingTexture : _bakingTextures) {
                    bakingTexture->abort();
                }

                checkIfTexturesFinished();
            }
        } else {
            // we have errors to attend to, so we don't do extra processing for this texture
            // but we do need to remove that TextureBaker from our list
            // and then check if we're done with all textures
            _bakingTextures.remove(bakedTexture->getTextureURL());

            checkIfTexturesFinished();
        }
    }
}

void ModelBaker::handleAbortedTexture() {
    // grab the texture bake that was aborted and remove it from our hash since we don't need to track it anymore
    TextureBaker* bakedTexture = qobject_cast<TextureBaker*>(sender());

    qDebug() << "Texture aborted: " << bakedTexture->getTextureURL();

    if (bakedTexture) {
        _bakingTextures.remove(bakedTexture->getTextureURL());
    }

    // since a texture we were baking aborted, our status is also aborted
    _shouldAbort.store(true);

    // abort any other ongoing texture bakes since we know we'll end up failing
    for (auto& bakingTexture : _bakingTextures) {
        bakingTexture->abort();
    }

    checkIfTexturesFinished();
}

QUrl ModelBaker::getTextureURL(const QFileInfo& textureFileInfo, bool isEmbedded) {
    QUrl urlToTexture;

    if (isEmbedded) {
        urlToTexture = _modelURL.toString() + "/" + textureFileInfo.filePath();
    } else {
        if (textureFileInfo.exists() && textureFileInfo.isFile()) {
            // set the texture URL to the local texture that we have confirmed exists
            urlToTexture = QUrl::fromLocalFile(textureFileInfo.absoluteFilePath());
        } else {
            // external texture that we'll need to download or find

            // this is a relative file path which will require different handling
            // depending on the location of the original model
            if (_modelURL.isLocalFile() && textureFileInfo.exists() && textureFileInfo.isFile()) {
                // the absolute path we ran into for the texture in the model exists on this machine
                // so use that file
                urlToTexture = QUrl::fromLocalFile(textureFileInfo.absoluteFilePath());
            } else {
                // we didn't find the texture on this machine at the absolute path
                // so assume that it is right beside the model to match the behaviour of interface
                urlToTexture = _modelURL.resolved(textureFileInfo.fileName());
            }
        }
    }

    return urlToTexture;
}

QString ModelBaker::texturePathRelativeToModel(QUrl modelURL, QUrl textureURL) {
    auto modelPath = modelURL.toString(QUrl::RemoveFilename | QUrl::RemoveQuery | QUrl::RemoveFragment);
    auto texturePath = textureURL.toString(QUrl::RemoveFilename | QUrl::RemoveQuery | QUrl::RemoveFragment);

    if (texturePath.startsWith(modelPath)) {
        // texture path is a child of the model path, return the texture path without the model path
        return texturePath.mid(modelPath.length());
    } else {
        // the texture path was not a child of the model path, return the empty string
        return "";
    }
}

void ModelBaker::checkIfTexturesFinished() {
    // check if we're done everything we need to do for this model
    // and emit our finished signal if we're done

    if (_bakingTextures.isEmpty()) {
        if (shouldStop()) {
            // if we're checking for completion but we have errors
            // that means one or more of our texture baking operations failed

            if (_pendingErrorEmission) {
                setIsFinished(true);
            }

            return;
        } else {
            qCDebug(model_baking) << "Finished baking, emitting finished" << _modelURL;

            texturesFinished();

            setIsFinished(true);
        }
    }
}

void ModelBaker::setWasAborted(bool wasAborted) {
    if (wasAborted != _wasAborted.load()) {
        Baker::setWasAborted(wasAborted);

        if (wasAborted) {
            qCDebug(model_baking) << "Aborted baking" << _modelURL;
        }
    }
}

void ModelBaker::texturesFinished() {
    embedTextureMetaData();
    exportScene();
}

void ModelBaker::embedTextureMetaData() {
    std::vector<FBXNode> embeddedTextureNodes;

    for (FBXNode& rootChild : _rootNode.children) {
        if (rootChild.name == "Objects") {
            qlonglong maxId = 0;
            for (auto &child : rootChild.children) {
                if (child.properties.length() == 3) {
                    maxId = std::max(maxId, child.properties[0].toLongLong());
                }
            }

            for (auto& object : rootChild.children) {
                if (object.name == "Texture") {
                    QVariant relativeFilename;
                    for (auto& child : object.children) {
                        if (child.name == "RelativeFilename") {
                            relativeFilename = child.properties[0];
                            break;
                        }
                    }

                    if (relativeFilename.isNull()
                        || !relativeFilename.toString().endsWith(BAKED_META_TEXTURE_SUFFIX)) {
                        continue;
                    }
                    if (object.properties.length() < 2) {
                        qWarning() << "Found texture with unexpected number of properties: " << object.name;
                        continue;
                    }

                    FBXNode videoNode;
                    videoNode.name = "Video";
                    videoNode.properties.append(++maxId);
                    videoNode.properties.append(object.properties[1]);
                    videoNode.properties.append("Clip");

                    QString bakedTextureFilePath {
                        _bakedOutputDir + "/" + relativeFilename.toString()
                    };

                    QFile textureFile { bakedTextureFilePath };
                    if (!textureFile.open(QIODevice::ReadOnly)) {
                        qWarning() << "Failed to open: " << bakedTextureFilePath;
                        continue;
                    }

                    videoNode.children.append({ "RelativeFilename", { relativeFilename }, { } });
                    videoNode.children.append({ "Content", { textureFile.readAll() }, { } });

                    rootChild.children.append(videoNode);

                    textureFile.close();
                }
            }
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
