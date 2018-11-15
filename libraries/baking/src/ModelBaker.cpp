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

#include <FBXReader.h>
#include <FBXWriter.h>

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

ModelBaker::ModelBaker(const QUrl& inputModelURL, TextureBakerThreadGetter inputTextureThreadGetter,
                       const QString& bakedOutputDirectory, const QString& originalOutputDirectory) :
    _modelURL(inputModelURL),
    _bakedOutputDir(bakedOutputDirectory),
    _originalOutputDir(originalOutputDirectory),
    _textureThreadGetter(inputTextureThreadGetter)
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

}

ModelBaker::~ModelBaker() {
    if (_modelTempDir.exists()) {
        if (!_modelTempDir.remove(_originalModelFilePath)) {
            qCWarning(model_baking) << "Failed to remove temporary copy of fbx file:" << _originalModelFilePath;
        }
        if (!_modelTempDir.rmdir(".")) {
            qCWarning(model_baking) << "Failed to remove temporary directory:" << _modelTempDir;
        }
    }
}

void ModelBaker::abort() {
    Baker::abort();

    // tell our underlying TextureBaker instances to abort
    // the ModelBaker will wait until all are aborted before emitting its own abort signal
    for (auto& textureBaker : _bakingTextures) {
        textureBaker->abort();
    }
}

bool ModelBaker::compressMesh(HFMMesh& mesh, bool hasDeformers, FBXNode& dracoMeshNode, GetMaterialIDCallback materialIDCallback) {
    if (mesh.wasCompressed) {
        handleError("Cannot re-bake a file that contains compressed mesh");
        return false;
    }

    Q_ASSERT(mesh.normals.size() == 0 || mesh.normals.size() == mesh.vertices.size());
    Q_ASSERT(mesh.colors.size() == 0 || mesh.colors.size() == mesh.vertices.size());
    Q_ASSERT(mesh.texCoords.size() == 0 || mesh.texCoords.size() == mesh.vertices.size());

    int64_t numTriangles{ 0 };
    for (auto& part : mesh.parts) {
        if ((part.quadTrianglesIndices.size() % 3) != 0 || (part.triangleIndices.size() % 3) != 0) {
            handleWarning("Found a mesh part with invalid index data, skipping");
            continue;
        }
        numTriangles += part.quadTrianglesIndices.size() / 3;
        numTriangles += part.triangleIndices.size() / 3;
    }

    if (numTriangles == 0) {
        return false;
    }

    draco::TriangleSoupMeshBuilder meshBuilder;

    meshBuilder.Start(numTriangles);

    bool hasNormals{ mesh.normals.size() > 0 };
    bool hasColors{ mesh.colors.size() > 0 };
    bool hasTexCoords{ mesh.texCoords.size() > 0 };
    bool hasTexCoords1{ mesh.texCoords1.size() > 0 };
    bool hasPerFaceMaterials = (materialIDCallback) ? (mesh.parts.size() > 1 || materialIDCallback(0) != 0 ) : true;
    bool needsOriginalIndices{ hasDeformers };

    int normalsAttributeID { -1 };
    int colorsAttributeID { -1 };
    int texCoordsAttributeID { -1 };
    int texCoords1AttributeID { -1 };
    int faceMaterialAttributeID { -1 };
    int originalIndexAttributeID { -1 };

    const int positionAttributeID = meshBuilder.AddAttribute(draco::GeometryAttribute::POSITION,
                                                             3, draco::DT_FLOAT32);
    if (needsOriginalIndices) {
        originalIndexAttributeID = meshBuilder.AddAttribute(
            (draco::GeometryAttribute::Type)DRACO_ATTRIBUTE_ORIGINAL_INDEX,
            1, draco::DT_INT32);
    }

    if (hasNormals) {
        normalsAttributeID = meshBuilder.AddAttribute(draco::GeometryAttribute::NORMAL,
                                                      3, draco::DT_FLOAT32);
    }
    if (hasColors) {
        colorsAttributeID = meshBuilder.AddAttribute(draco::GeometryAttribute::COLOR,
                                                     3, draco::DT_FLOAT32);
    }
    if (hasTexCoords) {
        texCoordsAttributeID = meshBuilder.AddAttribute(draco::GeometryAttribute::TEX_COORD,
                                                        2, draco::DT_FLOAT32);
    }
    if (hasTexCoords1) {
        texCoords1AttributeID = meshBuilder.AddAttribute(
            (draco::GeometryAttribute::Type)DRACO_ATTRIBUTE_TEX_COORD_1,
            2, draco::DT_FLOAT32);
    }
    if (hasPerFaceMaterials) {
        faceMaterialAttributeID = meshBuilder.AddAttribute(
            (draco::GeometryAttribute::Type)DRACO_ATTRIBUTE_MATERIAL_ID,
            1, draco::DT_UINT16);
    }

    auto partIndex = 0;
    draco::FaceIndex face;
    uint16_t materialID;
    
    for (auto& part : mesh.parts) {
        materialID = (materialIDCallback) ? materialIDCallback(partIndex) : partIndex;
        
        auto addFace = [&](QVector<int>& indices, int index, draco::FaceIndex face) {
            int32_t idx0 = indices[index];
            int32_t idx1 = indices[index + 1];
            int32_t idx2 = indices[index + 2];

            if (hasPerFaceMaterials) {
                meshBuilder.SetPerFaceAttributeValueForFace(faceMaterialAttributeID, face, &materialID);
            }

            meshBuilder.SetAttributeValuesForFace(positionAttributeID, face,
                                                  &mesh.vertices[idx0], &mesh.vertices[idx1],
                                                  &mesh.vertices[idx2]);

            if (needsOriginalIndices) {
                meshBuilder.SetAttributeValuesForFace(originalIndexAttributeID, face,
                                                      &mesh.originalIndices[idx0],
                                                      &mesh.originalIndices[idx1],
                                                      &mesh.originalIndices[idx2]);
            }
            if (hasNormals) {
                meshBuilder.SetAttributeValuesForFace(normalsAttributeID, face,
                                                      &mesh.normals[idx0], &mesh.normals[idx1],
                                                      &mesh.normals[idx2]);
            }
            if (hasColors) {
                meshBuilder.SetAttributeValuesForFace(colorsAttributeID, face,
                                                      &mesh.colors[idx0], &mesh.colors[idx1],
                                                      &mesh.colors[idx2]);
            }
            if (hasTexCoords) {
                meshBuilder.SetAttributeValuesForFace(texCoordsAttributeID, face,
                                                      &mesh.texCoords[idx0], &mesh.texCoords[idx1],
                                                      &mesh.texCoords[idx2]);
            }
            if (hasTexCoords1) {
                meshBuilder.SetAttributeValuesForFace(texCoords1AttributeID, face,
                                                      &mesh.texCoords1[idx0], &mesh.texCoords1[idx1],
                                                      &mesh.texCoords1[idx2]);
            }
        };

        for (int i = 0; (i + 2) < part.quadTrianglesIndices.size(); i += 3) {
            addFace(part.quadTrianglesIndices, i, face++);
        }

        for (int i = 0; (i + 2) < part.triangleIndices.size(); i += 3) {
            addFace(part.triangleIndices, i, face++);
        }

        partIndex++;
    }

    auto dracoMesh = meshBuilder.Finalize();

    if (!dracoMesh) {
        handleWarning("Failed to finalize the baking of a draco Geometry node");
        return false;
    }

    // we need to modify unique attribute IDs for custom attributes
    // so the attributes are easily retrievable on the other side
    if (hasPerFaceMaterials) {
        dracoMesh->attribute(faceMaterialAttributeID)->set_unique_id(DRACO_ATTRIBUTE_MATERIAL_ID);
    }

    if (hasTexCoords1) {
        dracoMesh->attribute(texCoords1AttributeID)->set_unique_id(DRACO_ATTRIBUTE_TEX_COORD_1);
    }

    if (needsOriginalIndices) {
        dracoMesh->attribute(originalIndexAttributeID)->set_unique_id(DRACO_ATTRIBUTE_ORIGINAL_INDEX);
    }

    draco::Encoder encoder;

    encoder.SetAttributeQuantization(draco::GeometryAttribute::POSITION, 14);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::TEX_COORD, 12);
    encoder.SetAttributeQuantization(draco::GeometryAttribute::NORMAL, 10);
    encoder.SetSpeedOptions(0, 5);

    draco::EncoderBuffer buffer;
    encoder.EncodeMeshToBuffer(*dracoMesh, &buffer);

    FBXNode dracoNode;
    dracoNode.name = "DracoMesh";
    auto value = QVariant::fromValue(QByteArray(buffer.data(), (int)buffer.size()));
    dracoNode.properties.append(value);
    
    dracoMeshNode = dracoNode;
    // Mesh compression successful return true
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
        auto urlToTexture = getTextureURL(modelTextureFileInfo, modelTextureFileName, !textureContent.isNull());

        QString baseTextureFileName;
        if (_remappedTexturePaths.contains(urlToTexture)) {
            baseTextureFileName = _remappedTexturePaths[urlToTexture];
        } else {
            // construct the new baked texture file name and file path
            // ensuring that the baked texture will have a unique name
            // even if there was another texture with the same name at a different path
            baseTextureFileName = createBaseTextureFileName(modelTextureFileInfo);
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

QUrl ModelBaker::getTextureURL(const QFileInfo& textureFileInfo, QString relativeFileName, bool isEmbedded) {
    QUrl urlToTexture;

    // use QFileInfo to easily split up the existing texture filename into its components
    auto apparentRelativePath = QFileInfo(relativeFileName.replace("\\", "/"));

    if (isEmbedded) {
        urlToTexture = _modelURL.toString() + "/" + apparentRelativePath.filePath();
    } else {
        if (textureFileInfo.exists() && textureFileInfo.isFile()) {
            // set the texture URL to the local texture that we have confirmed exists
            urlToTexture = QUrl::fromLocalFile(textureFileInfo.absoluteFilePath());
        } else {
            // external texture that we'll need to download or find

            // this is a relative file path which will require different handling
            // depending on the location of the original model
            if (_modelURL.isLocalFile() && apparentRelativePath.exists() && apparentRelativePath.isFile()) {
                // the absolute path we ran into for the texture in the model exists on this machine
                // so use that file
                urlToTexture = QUrl::fromLocalFile(apparentRelativePath.absoluteFilePath());
            } else {
                // we didn't find the texture on this machine at the absolute path
                // so assume that it is right beside the model to match the behaviour of interface
                urlToTexture = _modelURL.resolved(apparentRelativePath.fileName());
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

QString ModelBaker::createBaseTextureFileName(const QFileInfo& textureFileInfo) {
    // first make sure we have a unique base name for this texture
    // in case another texture referenced by this model has the same base name
    auto& nameMatches = _textureNameMatchCount[textureFileInfo.baseName()];

    QString baseTextureFileName{ textureFileInfo.completeBaseName() };

    if (nameMatches > 0) {
        // there are already nameMatches texture with this name
        // append - and that number to our baked texture file name so that it is unique
        baseTextureFileName += "-" + QString::number(nameMatches);
    }

    // increment the number of name matches
    ++nameMatches;

    return baseTextureFileName;
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
    // save the relative path to this FBX inside our passed output folder
    auto fileName = _modelURL.fileName();
    auto baseName = fileName.left(fileName.lastIndexOf('.'));
    auto bakedFilename = baseName + BAKED_FBX_EXTENSION;

    _bakedModelFilePath = _bakedOutputDir + "/" + bakedFilename;

    auto fbxData = FBXWriter::encodeFBX(_rootNode);

    QFile bakedFile(_bakedModelFilePath);

    if (!bakedFile.open(QIODevice::WriteOnly)) {
        handleError("Error opening " + _bakedModelFilePath + " for writing");
        return;
    }

    bakedFile.write(fbxData);

    _outputFiles.push_back(_bakedModelFilePath);

#ifdef HIFI_DUMP_FBX
    {
        FBXToJSON fbxToJSON;
        fbxToJSON << _rootNode;
        QFileInfo modelFile(_bakedModelFilePath);
        QString outFilename(modelFile.dir().absolutePath() + "/" + modelFile.completeBaseName() + "_FBX.json");
        QFile jsonFile(outFilename);
        if (jsonFile.open(QIODevice::WriteOnly)) {
            jsonFile.write(fbxToJSON.str().c_str(), fbxToJSON.str().length());
            jsonFile.close();
        }
    }
#endif

    qCDebug(model_baking) << "Exported" << _modelURL << "with re-written paths to" << _bakedModelFilePath;
}
