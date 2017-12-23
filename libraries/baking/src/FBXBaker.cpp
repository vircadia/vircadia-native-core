//
//  FBXBaker.cpp
//  tools/baking/src
//
//  Created by Stephen Birarda on 3/30/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <cmath> // need this include so we don't get an error looking for std::isnan

#include <QtConcurrent>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>

#include <mutex>

#include <NetworkAccessManager.h>
#include <SharedUtil.h>

#include <PathUtils.h>

#include <FBXReader.h>
#include <FBXWriter.h>

#include "ModelBakingLoggingCategory.h"
#include "TextureBaker.h"

#include "FBXBaker.h"

#ifdef _WIN32
#pragma warning( push )
#pragma warning( disable : 4267 )
#endif

#include <draco/mesh/triangle_soup_mesh_builder.h>
#include <draco/compression/encode.h>

#ifdef _WIN32
#pragma warning( pop )
#endif


FBXBaker::FBXBaker(const QUrl& fbxURL, TextureBakerThreadGetter textureThreadGetter,
                   const QString& bakedOutputDir, const QString& originalOutputDir) :
    _fbxURL(fbxURL),
    _bakedOutputDir(bakedOutputDir),
    _originalOutputDir(originalOutputDir),
    _textureThreadGetter(textureThreadGetter)
{

}

FBXBaker::~FBXBaker() {
    if (_tempDir.exists()) {
        if (!_tempDir.remove(_originalFBXFilePath)) {
            qCWarning(model_baking) << "Failed to remove temporary copy of fbx file:" << _originalFBXFilePath;
        }
        if (!_tempDir.rmdir(".")) {
            qCWarning(model_baking) << "Failed to remove temporary directory:" << _tempDir;
        }
    }
}

void FBXBaker::abort() {
    Baker::abort();

    // tell our underlying TextureBaker instances to abort
    // the FBXBaker will wait until all are aborted before emitting its own abort signal
    for (auto& textureBaker : _bakingTextures) {
        textureBaker->abort();
    }
}

void FBXBaker::bake() {
    qDebug() << "FBXBaker" << _fbxURL << "bake starting";
    
    auto tempDir = PathUtils::generateTemporaryDir();

    if (tempDir.isEmpty()) {
        handleError("Failed to create a temporary directory.");
        return;
    }

    _tempDir = tempDir;

    _originalFBXFilePath = _tempDir.filePath(_fbxURL.fileName());
    qDebug() << "Made temporary dir " << _tempDir;
    qDebug() << "Origin file path: " << _originalFBXFilePath;

    // setup the output folder for the results of this bake
    setupOutputFolder();

    if (shouldStop()) {
        return;
    }

    connect(this, &FBXBaker::sourceCopyReadyToLoad, this, &FBXBaker::bakeSourceCopy);

    // make a local copy of the FBX file
    loadSourceFBX();
}

void FBXBaker::bakeSourceCopy() {
    // load the scene from the FBX file
    importScene();

    if (shouldStop()) {
        return;
    }

    // enumerate the models and textures found in the scene and start a bake for them
    rewriteAndBakeSceneTextures();

    if (shouldStop()) {
        return;
    }

    rewriteAndBakeSceneModels();

    if (shouldStop()) {
        return;
    }

    // export the FBX with re-written texture references
    exportScene();

    if (shouldStop()) {
        return;
    }

    // check if we're already done with textures (in case we had none to re-write)
    checkIfTexturesFinished();
}

void FBXBaker::setupOutputFolder() {
    // make sure there isn't already an output directory using the same name
    if (QDir(_bakedOutputDir).exists()) {
        qWarning() << "Output path" << _bakedOutputDir << "already exists. Continuing.";
    } else {
        qCDebug(model_baking) << "Creating FBX output folder" << _bakedOutputDir;

        // attempt to make the output folder
        if (!QDir().mkpath(_bakedOutputDir)) {
            handleError("Failed to create FBX output folder " + _bakedOutputDir);
            return;
        }
        // attempt to make the output folder
        if (!QDir().mkpath(_originalOutputDir)) {
            handleError("Failed to create FBX output folder " + _bakedOutputDir);
            return;
        }
    }
}

void FBXBaker::loadSourceFBX() {
    // check if the FBX is local or first needs to be downloaded
    if (_fbxURL.isLocalFile()) {
        // load up the local file
        QFile localFBX { _fbxURL.toLocalFile() };

        qDebug() << "Local file url: " << _fbxURL << _fbxURL.toString() << _fbxURL.toLocalFile() << ", copying to: " << _originalFBXFilePath;

        if (!localFBX.exists()) {
            //QMessageBox::warning(this, "Could not find " + _fbxURL.toString(), "");
            handleError("Could not find " + _fbxURL.toString());
            return;
        }

        // make a copy in the output folder
        if (!_originalOutputDir.isEmpty()) {
            qDebug() << "Copying to: " << _originalOutputDir << "/" << _fbxURL.fileName();
            localFBX.copy(_originalOutputDir + "/" + _fbxURL.fileName());
        }

        localFBX.copy(_originalFBXFilePath);

        // emit our signal to start the import of the FBX source copy
        emit sourceCopyReadyToLoad();
    } else {
        // remote file, kick off a download
        auto& networkAccessManager = NetworkAccessManager::getInstance();

        QNetworkRequest networkRequest;

        // setup the request to follow re-directs and always hit the network
        networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
        networkRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
        networkRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);

        networkRequest.setUrl(_fbxURL);

        qCDebug(model_baking) << "Downloading" << _fbxURL;
        auto networkReply = networkAccessManager.get(networkRequest);

        connect(networkReply, &QNetworkReply::finished, this, &FBXBaker::handleFBXNetworkReply);
    }
}

void FBXBaker::handleFBXNetworkReply() {
    auto requestReply = qobject_cast<QNetworkReply*>(sender());

    if (requestReply->error() == QNetworkReply::NoError) {
        qCDebug(model_baking) << "Downloaded" << _fbxURL;

        // grab the contents of the reply and make a copy in the output folder
        QFile copyOfOriginal(_originalFBXFilePath);

        qDebug(model_baking) << "Writing copy of original FBX to" << _originalFBXFilePath << copyOfOriginal.fileName();

        if (!copyOfOriginal.open(QIODevice::WriteOnly)) {
            // add an error to the error list for this FBX stating that a duplicate of the original FBX could not be made
            handleError("Could not create copy of " + _fbxURL.toString() + " (Failed to open " + _originalFBXFilePath + ")");
            return;
        }
        if (copyOfOriginal.write(requestReply->readAll()) == -1) {
            handleError("Could not create copy of " + _fbxURL.toString() + " (Failed to write)");
            return;
        }

        // close that file now that we are done writing to it
        copyOfOriginal.close();

        if (!_originalOutputDir.isEmpty()) {
            copyOfOriginal.copy(_originalOutputDir + "/" + _fbxURL.fileName());
        }

        // emit our signal to start the import of the FBX source copy
        emit sourceCopyReadyToLoad();
    } else {
        // add an error to our list stating that the FBX could not be downloaded
        handleError("Failed to download " + _fbxURL.toString());
    }
}

void FBXBaker::importScene() {
    qDebug() << "file path: " << _originalFBXFilePath.toLocal8Bit().data() << QDir(_originalFBXFilePath).exists();

    QFile fbxFile(_originalFBXFilePath);
    if (!fbxFile.open(QIODevice::ReadOnly)) {
        handleError("Error opening " + _originalFBXFilePath + " for reading");
        return;
    }

    FBXReader reader;

    qCDebug(model_baking) << "Parsing" << _fbxURL;
    _rootNode = reader._rootNode = reader.parseFBX(&fbxFile);
    _geometry = reader.extractFBXGeometry({}, _fbxURL.toString());
    _textureContent = reader._textureContent;
}

QString texturePathRelativeToFBX(QUrl fbxURL, QUrl textureURL) {
    auto fbxPath = fbxURL.toString(QUrl::RemoveFilename | QUrl::RemoveQuery | QUrl::RemoveFragment);
    auto texturePath = textureURL.toString(QUrl::RemoveFilename | QUrl::RemoveQuery | QUrl::RemoveFragment);

    if (texturePath.startsWith(fbxPath)) {
        // texture path is a child of the FBX path, return the texture path without the fbx path
        return texturePath.mid(fbxPath.length());
    } else {
        // the texture path was not a child of the FBX path, return the empty string
        return "";
    }
}

QString FBXBaker::createBakedTextureFileName(const QFileInfo& textureFileInfo) {
    // first make sure we have a unique base name for this texture
    // in case another texture referenced by this model has the same base name
    auto& nameMatches = _textureNameMatchCount[textureFileInfo.baseName()];

    QString bakedTextureFileName { textureFileInfo.completeBaseName() };

    if (nameMatches > 0) {
        // there are already nameMatches texture with this name
        // append - and that number to our baked texture file name so that it is unique
        bakedTextureFileName += "-" + QString::number(nameMatches);
    }

    bakedTextureFileName += BAKED_TEXTURE_EXT;

    // increment the number of name matches
    ++nameMatches;

    return bakedTextureFileName;
}

QUrl FBXBaker::getTextureURL(const QFileInfo& textureFileInfo, QString relativeFileName, bool isEmbedded) {

    QUrl urlToTexture;

    auto apparentRelativePath = QFileInfo(relativeFileName.replace("\\", "/"));

    if (isEmbedded) {
        urlToTexture = _fbxURL.toString() + "/" + apparentRelativePath.filePath();
    } else  {
        if (textureFileInfo.exists() && textureFileInfo.isFile()) {
            // set the texture URL to the local texture that we have confirmed exists
            urlToTexture = QUrl::fromLocalFile(textureFileInfo.absoluteFilePath());
        } else {
            // external texture that we'll need to download or find

            // this is a relative file path which will require different handling
            // depending on the location of the original FBX
            if (_fbxURL.isLocalFile() && apparentRelativePath.exists() && apparentRelativePath.isFile()) {
                // the absolute path we ran into for the texture in the FBX exists on this machine
                // so use that file
                urlToTexture = QUrl::fromLocalFile(apparentRelativePath.absoluteFilePath());
            } else {
                // we didn't find the texture on this machine at the absolute path
                // so assume that it is right beside the FBX to match the behaviour of interface
                urlToTexture = _fbxURL.resolved(apparentRelativePath.fileName());
            }
        }
    }

    return urlToTexture;
}

void FBXBaker::rewriteAndBakeSceneModels() {
    unsigned int meshIndex = 0;
    bool hasDeformers { false };
    for (FBXNode& rootChild : _rootNode.children) {
        if (rootChild.name == "Objects") {
            for (FBXNode& objectChild : rootChild.children) {
                if (objectChild.name == "Deformer") {
                    hasDeformers = true;
                    break;
                }
            }
        }
        if (hasDeformers) {
            break;
        }
    }
    for (FBXNode& rootChild : _rootNode.children) {
        if (rootChild.name == "Objects") {
            for (FBXNode& objectChild : rootChild.children) {
                if (objectChild.name == "Geometry") {

                    // TODO Pull this out of _geometry instead so we don't have to reprocess it
                    auto extractedMesh = FBXReader::extractMesh(objectChild, meshIndex, false);
                    auto& mesh = extractedMesh.mesh;

                    if (mesh.wasCompressed) {
                        handleError("Cannot re-bake a file that contains compressed mesh");
                        return;
                    }

                    Q_ASSERT(mesh.normals.size() == 0 || mesh.normals.size() == mesh.vertices.size());
                    Q_ASSERT(mesh.colors.size() == 0 || mesh.colors.size() == mesh.vertices.size());
                    Q_ASSERT(mesh.texCoords.size() == 0 || mesh.texCoords.size() == mesh.vertices.size());

                    int64_t numTriangles { 0 };
                    for (auto& part : mesh.parts) {
                        if ((part.quadTrianglesIndices.size() % 3) != 0 || (part.triangleIndices.size() % 3) != 0) {
                            handleWarning("Found a mesh part with invalid index data, skipping");
                            continue;
                        }
                        numTriangles += part.quadTrianglesIndices.size() / 3;
                        numTriangles += part.triangleIndices.size() / 3;
                    }

                    if (numTriangles == 0) {
                        continue;
                    }

                    draco::TriangleSoupMeshBuilder meshBuilder;

                    meshBuilder.Start(numTriangles);

                    bool hasNormals { mesh.normals.size() > 0 };
                    bool hasColors { mesh.colors.size() > 0 };
                    bool hasTexCoords { mesh.texCoords.size() > 0 };
                    bool hasTexCoords1 { mesh.texCoords1.size() > 0 };
                    bool hasPerFaceMaterials { mesh.parts.size() > 1
                        || extractedMesh.partMaterialTextures[0].first != 0 };
                    bool needsOriginalIndices { hasDeformers };

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
                    for (auto& part : mesh.parts) {
                        const auto& matTex = extractedMesh.partMaterialTextures[partIndex];
                        uint16_t materialID = matTex.first;

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
                        continue;
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

                    FBXNode dracoMeshNode;
                    dracoMeshNode.name = "DracoMesh";
                    auto value = QVariant::fromValue(QByteArray(buffer.data(), (int) buffer.size()));
                    dracoMeshNode.properties.append(value);

                    objectChild.children.push_back(dracoMeshNode);

                    static const std::vector<QString> nodeNamesToDelete {
                        // Node data that is packed into the draco mesh
                        "Vertices",
                        "PolygonVertexIndex",
                        "LayerElementNormal",
                        "LayerElementColor",
                        "LayerElementUV",
                        "LayerElementMaterial",
                        "LayerElementTexture",

                        // Node data that we don't support
                        "Edges",
                        "LayerElementTangent",
                        "LayerElementBinormal",
                        "LayerElementSmoothing"
                    };
                    auto& children = objectChild.children;
                    auto it = children.begin();
                    while (it != children.end()) {
                        auto begin = nodeNamesToDelete.begin();
                        auto end = nodeNamesToDelete.end();
                        if (find(begin, end, it->name) != end) {
                            it = children.erase(it);
                        } else {
                            ++it;
                        }
                    }
                }
            }
        }
    }
}

void FBXBaker::rewriteAndBakeSceneTextures() {
    using namespace image::TextureUsage;
    QHash<QString, image::TextureUsage::Type> textureTypes;

    // enumerate the materials in the extracted geometry so we can determine the texture type for each texture ID
    for (const auto& material : _geometry->materials) {
        if (material.normalTexture.isBumpmap) {
            textureTypes[material.normalTexture.id] = BUMP_TEXTURE;
        } else {
            textureTypes[material.normalTexture.id] = NORMAL_TEXTURE;
        }

        textureTypes[material.albedoTexture.id] = ALBEDO_TEXTURE;
        textureTypes[material.glossTexture.id] = GLOSS_TEXTURE;
        textureTypes[material.roughnessTexture.id] = ROUGHNESS_TEXTURE;
        textureTypes[material.specularTexture.id] = SPECULAR_TEXTURE;
        textureTypes[material.metallicTexture.id] = METALLIC_TEXTURE;
        textureTypes[material.emissiveTexture.id] = EMISSIVE_TEXTURE;
        textureTypes[material.occlusionTexture.id] = OCCLUSION_TEXTURE;
        textureTypes[material.lightmapTexture.id] = LIGHTMAP_TEXTURE;
    }

    // enumerate the children of the root node
    for (FBXNode& rootChild : _rootNode.children) {

        if (rootChild.name == "Objects") {

            // enumerate the objects
            auto object = rootChild.children.begin();
            while (object != rootChild.children.end()) {
                if (object->name == "Texture") {

                    // double check that we didn't get an abort while baking the last texture
                    if (shouldStop()) {
                        return;
                    }

                    // enumerate the texture children
                    for (FBXNode& textureChild : object->children) {

                        if (textureChild.name == "RelativeFilename") {

                            // use QFileInfo to easily split up the existing texture filename into its components
                            QString fbxTextureFileName { textureChild.properties.at(0).toByteArray() };
                            QFileInfo textureFileInfo { fbxTextureFileName.replace("\\", "/") };

                            if (hasErrors()) {
                                return;
                            }

                            if (textureFileInfo.suffix() == BAKED_TEXTURE_EXT.mid(1)) {
                                // re-baking an FBX that already references baked textures is a fail
                                // so we add an error and return from here
                                handleError("Cannot re-bake a file that references compressed textures");

                                return;
                            }

                            if (!image::getSupportedFormats().contains(textureFileInfo.suffix())) {
                                // this is a texture format we don't bake, skip it
                                handleWarning(fbxTextureFileName + " is not a bakeable texture format");
                                continue;
                            }

                            // make sure this texture points to something and isn't one we've already re-mapped
                            if (!textureFileInfo.filePath().isEmpty()) {
                                // check if this was an embedded texture we have already have in-memory content for
                                auto textureContent = _textureContent.value(fbxTextureFileName.toLocal8Bit());

                                // figure out the URL to this texture, embedded or external
                                auto urlToTexture = getTextureURL(textureFileInfo, fbxTextureFileName,
                                                                  !textureContent.isNull());

                                QString bakedTextureFileName;
                                if (_remappedTexturePaths.contains(urlToTexture)) {
                                    bakedTextureFileName = _remappedTexturePaths[urlToTexture];
                                } else {
                                    // construct the new baked texture file name and file path
                                    // ensuring that the baked texture will have a unique name
                                    // even if there was another texture with the same name at a different path
                                    bakedTextureFileName = createBakedTextureFileName(textureFileInfo);
                                    _remappedTexturePaths[urlToTexture] = bakedTextureFileName;
                                }

                                qCDebug(model_baking).noquote() << "Re-mapping" << fbxTextureFileName
                                    << "to" << bakedTextureFileName;

                                QString bakedTextureFilePath {
                                    _bakedOutputDir + "/" + bakedTextureFileName
                                };

                                // write the new filename into the FBX scene
                                textureChild.properties[0] = bakedTextureFileName.toLocal8Bit();

                                if (!_bakingTextures.contains(urlToTexture)) {
                                    _outputFiles.push_back(bakedTextureFilePath);

                                    // grab the ID for this texture so we can figure out the
                                    // texture type from the loaded materials
                                    QString textureID { object->properties[0].toByteArray() };
                                    auto textureType = textureTypes[textureID];

                                    // bake this texture asynchronously
                                    bakeTexture(urlToTexture, textureType, _bakedOutputDir, bakedTextureFileName, textureContent);
                                }
                            }
                        }
                    }

                    ++object;

                } else if (object->name == "Video") {
                    // this is an embedded texture, we need to remove it from the FBX
                    object = rootChild.children.erase(object);
                } else {
                    ++object;
                }
            }
        }
    }
}

void FBXBaker::bakeTexture(const QUrl& textureURL, image::TextureUsage::Type textureType,
                           const QDir& outputDir, const QString& bakedFilename, const QByteArray& textureContent) {
    // start a bake for this texture and add it to our list to keep track of
    QSharedPointer<TextureBaker> bakingTexture {
        new TextureBaker(textureURL, textureType, outputDir, bakedFilename, textureContent),
        &TextureBaker::deleteLater
    };

    // make sure we hear when the baking texture is done or aborted
    connect(bakingTexture.data(), &Baker::finished, this, &FBXBaker::handleBakedTexture);
    connect(bakingTexture.data(), &TextureBaker::aborted, this, &FBXBaker::handleAbortedTexture);

    // keep a shared pointer to the baking texture
    _bakingTextures.insert(textureURL, bakingTexture);

    // start baking the texture on one of our available worker threads
    bakingTexture->moveToThread(_textureThreadGetter());
    QMetaObject::invokeMethod(bakingTexture.data(), "bake");
}

void FBXBaker::handleBakedTexture() {
    TextureBaker* bakedTexture = qobject_cast<TextureBaker*>(sender());

    // make sure we haven't already run into errors, and that this is a valid texture
    if (bakedTexture) {
        if (!shouldStop()) {
            if (!bakedTexture->hasErrors()) {
                if (!_originalOutputDir.isEmpty()) {
                    // we've been asked to make copies of the originals, so we need to make copies of this if it is a linked texture

                    // use the path to the texture being baked to determine if this was an embedded or a linked texture

                    // it is embeddded if the texure being baked was inside a folder with the name of the FBX
                    // since that is the fake URL we provide when baking external textures

                    if (!_fbxURL.isParentOf(bakedTexture->getTextureURL())) {
                        // for linked textures we want to save a copy of original texture beside the original FBX

                        qCDebug(model_baking) << "Saving original texture for" << bakedTexture->getTextureURL();

                        // check if we have a relative path to use for the texture
                        auto relativeTexturePath = texturePathRelativeToFBX(_fbxURL, bakedTexture->getTextureURL());

                        QFile originalTextureFile {
                            _originalOutputDir + "/" + relativeTexturePath + bakedTexture->getTextureURL().fileName()
                        };

                        if (relativeTexturePath.length() > 0) {
                            // make the folders needed by the relative path
                        }

                        if (originalTextureFile.open(QIODevice::WriteOnly) && originalTextureFile.write(bakedTexture->getOriginalTexture()) != -1) {
                            qCDebug(model_baking) << "Saved original texture file" << originalTextureFile.fileName()
                                << "for" << _fbxURL;
                        } else {
                            handleError("Could not save original external texture " + originalTextureFile.fileName()
                                        + " for " + _fbxURL.toString());
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

void FBXBaker::handleAbortedTexture() {
    // grab the texture bake that was aborted and remove it from our hash since we don't need to track it anymore
    TextureBaker* bakedTexture = qobject_cast<TextureBaker*>(sender());

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

void FBXBaker::exportScene() {
    // save the relative path to this FBX inside our passed output folder
    auto fileName = _fbxURL.fileName();
    auto baseName = fileName.left(fileName.lastIndexOf('.'));
    auto bakedFilename = baseName + BAKED_FBX_EXTENSION;

    _bakedFBXFilePath = _bakedOutputDir + "/" + bakedFilename;

    auto fbxData = FBXWriter::encodeFBX(_rootNode);

    QFile bakedFile(_bakedFBXFilePath);

    if (!bakedFile.open(QIODevice::WriteOnly)) {
        handleError("Error opening " + _bakedFBXFilePath + " for writing");
        return;
    }

    bakedFile.write(fbxData);

    _outputFiles.push_back(_bakedFBXFilePath);

    qCDebug(model_baking) << "Exported" << _fbxURL << "with re-written paths to" << _bakedFBXFilePath;
}

void FBXBaker::checkIfTexturesFinished() {
    // check if we're done everything we need to do for this FBX
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
            qCDebug(model_baking) << "Finished baking, emitting finsihed" << _fbxURL;

            setIsFinished(true);
        }
    }
}

void FBXBaker::setWasAborted(bool wasAborted) {
    if (wasAborted != _wasAborted.load()) {
        Baker::setWasAborted(wasAborted);

        if (wasAborted) {
            qCDebug(model_baking) << "Aborted baking" << _fbxURL;
        }
    }
}
