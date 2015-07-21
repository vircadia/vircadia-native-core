//
//  OBJReader.cpp
//  libraries/fbx/src/
//
//  Created by Seth Alves on 3/7/15.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
// http://en.wikipedia.org/wiki/Wavefront_.obj_file
// http://www.scratchapixel.com/old/lessons/3d-advanced-lessons/obj-file-format/obj-file-format/
// http://paulbourke.net/dataformats/obj/


#include <QBuffer>
#include <QIODevice>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QEventLoop>
#include <ctype.h>  // .obj files are not locale-specific. The C/ASCII charset applies.

#include <NetworkAccessManager.h>
#include "FBXReader.h"
#include "OBJReader.h"
#include "Shape.h"
#include "ModelFormatLogging.h"


QHash<QString, float> COMMENT_SCALE_HINTS = {{"This file uses centimeters as units", 1.0f / 100.0f},
                                             {"This file uses millimeters as units", 1.0f / 1000.0f}};

const QString SMART_DEFAULT_MATERIAL_NAME = "High Fidelity smart default material name";

OBJTokenizer::OBJTokenizer(QIODevice* device) : _device(device), _pushedBackToken(-1) {
}

const QByteArray OBJTokenizer::getLineAsDatum() {
    return _device->readLine().trimmed();
}

int OBJTokenizer::nextToken() {
    if (_pushedBackToken != NO_PUSHBACKED_TOKEN) {
        int token = _pushedBackToken;
        _pushedBackToken = NO_PUSHBACKED_TOKEN;
        return token;
    }

    char ch;
    while (_device->getChar(&ch)) {
        if (QChar(ch).isSpace()) {
            continue; // skip whitespace
        }
        switch (ch) {
            case '#': {
                _comment = _device->readLine(); // stash comment for a future call to getComment
                return COMMENT_TOKEN;
            }

            case '\"':
                _datum = "";
                while (_device->getChar(&ch)) {
                    if (ch == '\"') { // end on closing quote
                        break;
                    }
                    if (ch == '\\') { // handle escaped quotes
                        if (_device->getChar(&ch) && ch != '\"') {
                            _datum.append('\\');
                        }
                    }
                    _datum.append(ch);
                }
                return DATUM_TOKEN;

            default:
                _datum = "";
                _datum.append(ch);
                while (_device->getChar(&ch)) {
                    if (QChar(ch).isSpace() || ch == '\"') {
                        ungetChar(ch); // read until we encounter a special character, then replace it
                        break;
                    }
                    _datum.append(ch);
                }

                return DATUM_TOKEN;
        }
    }
    return NO_TOKEN;
}

bool OBJTokenizer::isNextTokenFloat() {
    if (nextToken() != OBJTokenizer::DATUM_TOKEN) {
        return false;
    }
    QByteArray token = getDatum();
    pushBackToken(OBJTokenizer::DATUM_TOKEN);
    bool ok;
    token.toFloat(&ok);
    return ok;
}

glm::vec3 OBJTokenizer::getVec3() {
    auto x = getFloat(); // N.B.: getFloat() has side-effect
    auto y = getFloat(); // And order of arguments is different on Windows/Linux.
    auto z = getFloat();
    auto v = glm::vec3(x, y, z);
    while (isNextTokenFloat()) {
        // the spec(s) get(s) vague here.  might be w, might be a color... chop it off.
        nextToken();
    }
    return v;
}
glm::vec2 OBJTokenizer::getVec2() {
    auto v = glm::vec2(getFloat(), 1.0f - getFloat());  // OBJ has an odd sense of u, v. Also N.B.: getFloat() has side-effect
    while (isNextTokenFloat()) {
        // there can be a w, but we don't handle that
        nextToken();
    }
    return v;
}


void setMeshPartDefaults(FBXMeshPart& meshPart, QString materialID) {
    meshPart.diffuseColor = glm::vec3(1, 1, 1);
    meshPart.specularColor = glm::vec3(1, 1, 1);
    meshPart.emissiveColor = glm::vec3(0, 0, 0);
    meshPart.emissiveParams = glm::vec2(0, 1);
    meshPart.shininess = 40;
    meshPart.opacity = 1;
 
    meshPart.materialID = materialID;
    meshPart.opacity = 1.0;
    meshPart._material = std::make_shared<model::Material>();
    meshPart._material->setDiffuse(glm::vec3(1.0, 1.0, 1.0));
    meshPart._material->setOpacity(1.0);
    meshPart._material->setMetallic(0.0);
    meshPart._material->setGloss(96.0);
    meshPart._material->setEmissive(glm::vec3(0.0, 0.0, 0.0));
}

// OBJFace
bool OBJFace::add(const QByteArray& vertexIndex, const QByteArray& textureIndex, const QByteArray& normalIndex, const QVector<glm::vec3>& vertices) {
    bool ok;
    int index = vertexIndex.toInt(&ok);
    if (!ok) {
        return false;
    }
    vertexIndices.append(index - 1);
    if (!textureIndex.isEmpty()) {
        index = textureIndex.toInt(&ok);
        if (!ok) {
            return false;
        }
        if (index < 0) { // Count backwards from the last one added.
            index = vertices.count() + 1 + index;
        }
        textureUVIndices.append(index - 1);
    }
    if (!normalIndex.isEmpty()) {
        index = normalIndex.toInt(&ok);
        if (!ok) {
            return false;
        }
        normalIndices.append(index - 1);
    }
    return true;
}
QVector<OBJFace> OBJFace::triangulate() {
    QVector<OBJFace> newFaces;
    const int nVerticesInATriangle = 3;
    if (vertexIndices.count() == nVerticesInATriangle) {
        newFaces.append(*this);
    } else {
        for (int i = 1; i < vertexIndices.count() - 1; i++) {
            OBJFace newFace;
            newFace.addFrom(this, 0);
            newFace.addFrom(this, i);
            newFace.addFrom(this, i + 1);
            newFace.groupName = groupName;
            newFace.materialName = materialName;
            newFaces.append(newFace);
        }
    }
    return newFaces;
}
void OBJFace::addFrom(const OBJFace* face, int index) { // add using data from f at index i
    vertexIndices.append(face->vertexIndices[index]);
    if (face->textureUVIndices.count() > 0) { // Any at all. Runtime error if not consistent.
        textureUVIndices.append(face->textureUVIndices[index]);
    }
    if (face->normalIndices.count() > 0) {
        normalIndices.append(face->normalIndices[index]);
    }
}

bool OBJReader::isValidTexture(const QByteArray &filename) {
    QUrl candidateUrl = url->resolved(QUrl(filename));
    QNetworkReply *netReply = request(candidateUrl, true);
    bool isValid = netReply->isFinished() && (netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200);
    netReply->deleteLater();
    return isValid;
}

void OBJReader::parseMaterialLibrary(QIODevice* device) {
    OBJTokenizer tokenizer(device);
    QString matName = SMART_DEFAULT_MATERIAL_NAME;
    OBJMaterial& currentMaterial = materials[matName];
    while (true) {
        switch (tokenizer.nextToken()) {
            case OBJTokenizer::COMMENT_TOKEN:
                qCDebug(modelformat) << "OBJ Reader MTLLIB comment:" << tokenizer.getComment();
                break;
            case OBJTokenizer::DATUM_TOKEN:
                break;
            default:
                materials[matName] = currentMaterial;
                qCDebug(modelformat) << "OBJ Reader Last material shininess:" << currentMaterial.shininess << " opacity:" << currentMaterial.opacity << " diffuse color:" << currentMaterial.diffuseColor << " specular color:" << currentMaterial.specularColor << " diffuse texture:" << currentMaterial.diffuseTextureFilename << " specular texture:" << currentMaterial.specularTextureFilename;
                return;
        }
        QByteArray token = tokenizer.getDatum();
        if (token == "newmtl") {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                return;
            }
            materials[matName] = currentMaterial;
            matName = tokenizer.getDatum();
            currentMaterial = materials[matName];
            currentMaterial.diffuseTextureFilename = "test";
            qCDebug(modelformat) << "OBJ Reader Starting new material definition " << matName;
            currentMaterial.diffuseTextureFilename = "";
        } else if (token == "Ns") {
            currentMaterial.shininess = tokenizer.getFloat();
        } else if ((token == "d") || (token == "Tr")) {
            currentMaterial.opacity = tokenizer.getFloat();
        } else if (token == "Ka") {
            qCDebug(modelformat) << "OBJ Reader Ignoring material Ka " << tokenizer.getVec3();
        } else if (token == "Kd") {
            currentMaterial.diffuseColor = tokenizer.getVec3();
        } else if (token == "Ks") {
            currentMaterial.specularColor = tokenizer.getVec3();
        } else if ((token == "map_Kd") || (token == "map_Ks")) {
            QByteArray filename = QUrl(tokenizer.getLineAsDatum()).fileName().toUtf8();
            if (filename.endsWith(".tga")) {
                qCDebug(modelformat) << "OBJ Reader WARNING: currently ignoring tga texture " << filename << " in " << url;
                break;
            }
            if (isValidTexture(filename)) {
                if (token == "map_Kd") {
                    currentMaterial.diffuseTextureFilename = filename;
                } else {
                    currentMaterial.specularTextureFilename = filename;
                }
            } else {
                qCDebug(modelformat) << "OBJ Reader WARNING: " << url << " ignoring missing texture " << filename;
            }
        }
    }
}

QNetworkReply* OBJReader::request(QUrl& url, bool isTest) {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest netRequest(url);
    QNetworkReply* netReply = isTest ? networkAccessManager.head(netRequest) : networkAccessManager.get(netRequest);
    QEventLoop loop; // Create an event loop that will quit when we get the finished signal
    QObject::connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();                    // Nothing is going to happen on this whole run thread until we get this
    netReply->waitForReadyRead(-1); // so we might as well block this thread waiting for the response, rather than
    return netReply;                // trying to sync later on.
}


bool OBJReader::parseOBJGroup(OBJTokenizer& tokenizer, const QVariantHash& mapping, FBXGeometry& geometry, float& scaleGuess) {
    FaceGroup faces;
    FBXMesh& mesh = geometry.meshes[0];
    mesh.parts.append(FBXMeshPart());
    FBXMeshPart& meshPart = mesh.parts.last();
    bool sawG = false;
    bool result = true;
    int originalFaceCountForDebugging = 0;
    QString currentGroup;

    setMeshPartDefaults(meshPart, QString("dontknow") + QString::number(mesh.parts.count()));

    while (true) {
        int tokenType = tokenizer.nextToken();
        if (tokenType == OBJTokenizer::COMMENT_TOKEN) {
            // loop through the list of known comments which suggest a scaling factor.
            // if we find one, save the scaling hint into scaleGuess
            QString comment = tokenizer.getComment();
            QHashIterator<QString, float> i(COMMENT_SCALE_HINTS);
            while (i.hasNext()) {
                i.next();
                if (comment.contains(i.key())) {
                    scaleGuess = i.value();
                }
            }
            continue;
        }
        if (tokenType != OBJTokenizer::DATUM_TOKEN) {
            result = false;
            break;
        }
        QByteArray token = tokenizer.getDatum();
        //qCDebug(modelformat) << token;
        if (token == "g") {
            if (sawG) {
                // we've encountered the beginning of the next group.
                tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                break;
            }
            sawG = true;
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            QByteArray groupName = tokenizer.getDatum();
            currentGroup = groupName;
            //qCDebug(modelformat) << "new group:" << groupName;
        } else if (token == "mtllib") {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            QByteArray libraryName = tokenizer.getDatum();
            if (librariesSeen.contains(libraryName)) {
                break; // Some files use mtllib over and over again for the same libraryName
            }
            librariesSeen[libraryName] = true;
            QUrl libraryUrl = url->resolved(QUrl(libraryName).fileName()); // Throw away any path part of libraryName, and merge against original url.
            qCDebug(modelformat) << "OBJ Reader new library:" << libraryName << " at:" << libraryUrl;
            QNetworkReply* netReply = request(libraryUrl, false);
            if (netReply->isFinished() && (netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)) {
                parseMaterialLibrary(netReply);
            } else {
                qCDebug(modelformat) << "OBJ Reader " << libraryName << " did not answer. Got " << netReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
            }
            netReply->deleteLater();
        } else if (token == "usemtl") {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            currentMaterialName = tokenizer.getDatum();
            qCDebug(modelformat) << "OBJ Reader new current material:" << currentMaterialName;
        } else if (token == "v") {
            vertices.append(tokenizer.getVec3());
        } else if (token == "vn") {
            normals.append(tokenizer.getVec3());
        } else if (token == "vt") {
            textureUVs.append(tokenizer.getVec2());
        } else if (token == "f") {
            OBJFace face;
            while (true) {
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                    if (face.vertexIndices.count() == 0) {
                        // nonsense, bail out.
                        goto done;
                    }
                    break;
                }
                // faces can be:
                //   vertex-index
                //   vertex-index/texture-index
                //   vertex-index/texture-index/surface-normal-index
                QByteArray token = tokenizer.getDatum();
                if (!isdigit(token[0])) { // Tokenizer treats line endings as whitespace. Non-digit indicates done;
                    tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                    break;
                }
                QList<QByteArray> parts = token.split('/');
                assert(parts.count() >= 1);
                assert(parts.count() <= 3);
                const QByteArray noData {};
                face.add(parts[0], (parts.count() > 1) ? parts[1] : noData, (parts.count() > 2) ? parts[2] : noData, vertices);
                face.groupName = currentGroup;
                face.materialName = currentMaterialName;
            }
            originalFaceCountForDebugging++;
            foreach(OBJFace face, face.triangulate()) {
                faces.append(face);
            }
        } else {
            // something we don't (yet) care about
            // qCDebug(modelformat) << "OBJ parser is skipping a line with" << token;
            tokenizer.skipLine();
        }
    }
done:
    if (faces.count() == 0) { // empty mesh
        mesh.parts.pop_back();
    } else {
        faceGroups.append(faces); // We're done with this group. Add the faces.
    }
    //qCDebug(modelformat) << "end group:" << meshPart.materialID << " original faces:" << originalFaceCountForDebugging << " triangles:" << faces.count() << " keep going:" << result;
    return result;
}


FBXGeometry OBJReader::readOBJ(const QByteArray& model, const QVariantHash& mapping) {
    QBuffer buffer(const_cast<QByteArray*>(&model));
    buffer.open(QIODevice::ReadOnly);
    return readOBJ(&buffer, mapping, nullptr);
}


FBXGeometry OBJReader::readOBJ(QIODevice* device, const QVariantHash& mapping, QUrl* url) {
    FBXGeometry geometry;
    OBJTokenizer tokenizer(device);
    float scaleGuess = 1.0f;

    this->url = url;
    geometry.meshExtents.reset();
    geometry.meshes.append(FBXMesh());
    
    try {
        // call parseOBJGroup as long as it's returning true.  Each successful call will
        // add a new meshPart to the geometry's single mesh.
        while (parseOBJGroup(tokenizer, mapping, geometry, scaleGuess)) {}

        FBXMesh& mesh = geometry.meshes[0];
        mesh.meshIndex = 0;
 
        geometry.joints.resize(1);
        geometry.joints[0].isFree = false;
        geometry.joints[0].parentIndex = -1;
        geometry.joints[0].distanceToParent = 0;
        geometry.joints[0].boneRadius = 0;
        geometry.joints[0].translation = glm::vec3(0, 0, 0);
        geometry.joints[0].rotationMin = glm::vec3(0, 0, 0);
        geometry.joints[0].rotationMax = glm::vec3(0, 0, 0);
        geometry.joints[0].name = "OBJ";
        geometry.joints[0].shapePosition = glm::vec3(0, 0, 0);
        geometry.joints[0].shapeType = SPHERE_SHAPE;
        geometry.joints[0].isSkeletonJoint = true;

        geometry.jointIndices["x"] = 1;

        FBXCluster cluster;
        cluster.jointIndex = 0;
        cluster.inverseBindMatrix = glm::mat4(1, 0, 0, 0,
                                              0, 1, 0, 0,
                                              0, 0, 1, 0,
                                              0, 0, 0, 1);
        mesh.clusters.append(cluster);
        
        // Some .obj files use the convention that a group with uv coordinates that doesn't define a material, should use a texture with the same basename as the .obj file.
        QString filename = url->fileName();
        int extIndex = filename.lastIndexOf('.'); // by construction, this does not fail
        QString basename = filename.remove(extIndex + 1, sizeof("obj"));
        OBJMaterial& preDefinedMaterial = materials[SMART_DEFAULT_MATERIAL_NAME];
        preDefinedMaterial.diffuseColor = glm::vec3(1.0f);
        QVector<QByteArray> extensions = {"jpg", "jpeg", "png", "tga"};
        QByteArray base = basename.toUtf8(), textName = "";
        for (int i = 0; i < extensions.count(); i++) {
            QByteArray candidateString = base + extensions[i];
            if (isValidTexture(candidateString)) {
                textName = candidateString;
                break;
            }
        }
        if (!textName.isEmpty()) {
            preDefinedMaterial.diffuseTextureFilename = textName;
        }
        materials[SMART_DEFAULT_MATERIAL_NAME] = preDefinedMaterial;
        
        for (int i = 0, meshPartCount = 0; i < mesh.parts.count(); i++, meshPartCount++) {
            FBXMeshPart& meshPart = mesh.parts[i];
            FaceGroup faceGroup = faceGroups[meshPartCount];
            OBJFace leadFace = faceGroup[0]; // All the faces in the same group will have the same name and material.
            QString groupMaterialName = leadFace.materialName;
            if (groupMaterialName.isEmpty() && (leadFace.textureUVIndices.count() > 0)) {
                qCDebug(modelformat) << "OBJ Reader WARNING: " << url << " needs a texture that isn't specified. Using default mechanism.";
                groupMaterialName = SMART_DEFAULT_MATERIAL_NAME;
            } else if (!groupMaterialName.isEmpty() && !materials.contains(groupMaterialName)) {
                qCDebug(modelformat) << "OBJ Reader WARNING: " << url << " specifies a material " << groupMaterialName << " that is not defined. Using default mechanism.";
                groupMaterialName = SMART_DEFAULT_MATERIAL_NAME;
            }
            if  (!groupMaterialName.isEmpty()) {
                OBJMaterial* material = &materials[groupMaterialName];
                // The code behind this is in transition. Some things are set directly in the FXBMeshPart...
                meshPart.materialID = groupMaterialName;
                meshPart.diffuseTexture.filename = material->diffuseTextureFilename;
                meshPart.specularTexture.filename = material->specularTextureFilename;
                // ... and some things are set in the underlying material.
                meshPart._material->setDiffuse(material->diffuseColor);
                meshPart._material->setMetallic(glm::length(material->specularColor));
                meshPart._material->setGloss(material->shininess);
                meshPart._material->setOpacity(material->opacity);
            }
            // qCDebug(modelformat) << "OBJ Reader part:" << meshPartCount << "name:" << leadFace.groupName << "material:" << groupMaterialName << "diffuse:" << meshPart._material->getDiffuse() << "faces:" << faceGroup.count() << "triangle indices will start with:" << mesh.vertices.count();
            foreach(OBJFace face, faceGroup) {
                glm::vec3 v0 = vertices[face.vertexIndices[0]];
                glm::vec3 v1 = vertices[face.vertexIndices[1]];
                glm::vec3 v2 = vertices[face.vertexIndices[2]];
                meshPart.triangleIndices.append(mesh.vertices.count()); // not face.vertexIndices into vertices
                mesh.vertices << v0;
                meshPart.triangleIndices.append(mesh.vertices.count());
                mesh.vertices << v1;
                meshPart.triangleIndices.append(mesh.vertices.count());
                mesh.vertices << v2;
                
                glm::vec3 n0, n1, n2;
                if (face.normalIndices.count()) {
                    n0 = normals[face.normalIndices[0]];
                    n1 = normals[face.normalIndices[1]];
                    n2 = normals[face.normalIndices[2]];
                } else { // generate normals from triangle plane if not provided
                    n0 = n1 = n2 = glm::cross(v1 - v0, v2 - v0);
                }
                mesh.normals << n0 << n1 << n2;
                if (face.textureUVIndices.count()) {
                    mesh.texCoords
                    << textureUVs[face.textureUVIndices[0]]
                    << textureUVs[face.textureUVIndices[1]]
                    << textureUVs[face.textureUVIndices[2]];
                } else {
                    glm::vec2 corner(0.0f, 1.0f);
                    mesh.texCoords << corner << corner << corner;
                }
            }
        }

        // if we got a hint about units, scale all the points
        if (scaleGuess != 1.0f) {
            for (int i = 0; i < mesh.vertices.size(); i++) {
                mesh.vertices[i] *= scaleGuess;
            }
        }
            
        mesh.meshExtents.reset();
        foreach (const glm::vec3& vertex, mesh.vertices) {
            mesh.meshExtents.addPoint(vertex);
            geometry.meshExtents.addPoint(vertex);
        }
        // fbxDebugDump(geometry);
    } catch(const std::exception& e) {
        qCDebug(modelformat) << "OBJ reader fail: " << e.what();
    }

    return geometry;
}



void fbxDebugDump(const FBXGeometry& fbxgeo) {
    qCDebug(modelformat) << "---------------- fbxGeometry ----------------";
    qCDebug(modelformat) << "  hasSkeletonJoints =" << fbxgeo.hasSkeletonJoints;
    qCDebug(modelformat) << "  offset =" << fbxgeo.offset;
    qCDebug(modelformat) << "  meshes.count() =" << fbxgeo.meshes.count();
    foreach (FBXMesh mesh, fbxgeo.meshes) {
        qCDebug(modelformat) << "    vertices.count() =" << mesh.vertices.count();
        qCDebug(modelformat) << "    normals.count() =" << mesh.normals.count();
        /*if (mesh.normals.count() == mesh.vertices.count()) {
            for (int i = 0; i < mesh.normals.count(); i++) {
                qCDebug(modelformat) << "        " << mesh.vertices[ i ] << mesh.normals[ i ];
            }
        }*/
        qCDebug(modelformat) << "    tangents.count() =" << mesh.tangents.count();
        qCDebug(modelformat) << "    colors.count() =" << mesh.colors.count();
        qCDebug(modelformat) << "    texCoords.count() =" << mesh.texCoords.count();
        qCDebug(modelformat) << "    texCoords1.count() =" << mesh.texCoords1.count();
        qCDebug(modelformat) << "    clusterIndices.count() =" << mesh.clusterIndices.count();
        qCDebug(modelformat) << "    clusterWeights.count() =" << mesh.clusterWeights.count();
        qCDebug(modelformat) << "    meshExtents =" << mesh.meshExtents;
        qCDebug(modelformat) << "    modelTransform =" << mesh.modelTransform;
        qCDebug(modelformat) << "    parts.count() =" << mesh.parts.count();
        foreach (FBXMeshPart meshPart, mesh.parts) {
            qCDebug(modelformat) << "        quadIndices.count() =" << meshPart.quadIndices.count();
            qCDebug(modelformat) << "        triangleIndices.count() =" << meshPart.triangleIndices.count();
            qCDebug(modelformat) << "        diffuseColor =" << meshPart.diffuseColor << "mat =" << meshPart._material->getDiffuse();
            qCDebug(modelformat) << "        specularColor =" << meshPart.specularColor << "mat =" << meshPart._material->getMetallic();
            qCDebug(modelformat) << "        emissiveColor =" << meshPart.emissiveColor << "mat =" << meshPart._material->getEmissive();
            qCDebug(modelformat) << "        emissiveParams =" << meshPart.emissiveParams;
            qCDebug(modelformat) << "        gloss =" << meshPart.shininess << "mat =" << meshPart._material->getGloss();
            qCDebug(modelformat) << "        opacity =" << meshPart.opacity << "mat =" << meshPart._material->getOpacity();
            qCDebug(modelformat) << "        materialID =" << meshPart.materialID;
            qCDebug(modelformat) << "        diffuse texture =" << meshPart.diffuseTexture.filename;
            qCDebug(modelformat) << "        specular texture =" << meshPart.specularTexture.filename;
        }
        qCDebug(modelformat) << "    clusters.count() =" << mesh.clusters.count();
        foreach (FBXCluster cluster, mesh.clusters) {
            qCDebug(modelformat) << "        jointIndex =" << cluster.jointIndex;
            qCDebug(modelformat) << "        inverseBindMatrix =" << cluster.inverseBindMatrix;
        }
    }

    qCDebug(modelformat) << "  jointIndices =" << fbxgeo.jointIndices;
    qCDebug(modelformat) << "  joints.count() =" << fbxgeo.joints.count();

    foreach (FBXJoint joint, fbxgeo.joints) {
        qCDebug(modelformat) << "    isFree =" << joint.isFree;
        qCDebug(modelformat) << "    freeLineage" << joint.freeLineage;
        qCDebug(modelformat) << "    parentIndex" << joint.parentIndex;
        qCDebug(modelformat) << "    distanceToParent" << joint.distanceToParent;
        qCDebug(modelformat) << "    boneRadius" << joint.boneRadius;
        qCDebug(modelformat) << "    translation" << joint.translation;
        qCDebug(modelformat) << "    preTransform" << joint.preTransform;
        qCDebug(modelformat) << "    preRotation" << joint.preRotation;
        qCDebug(modelformat) << "    rotation" << joint.rotation;
        qCDebug(modelformat) << "    postRotation" << joint.postRotation;
        qCDebug(modelformat) << "    postTransform" << joint.postTransform;
        qCDebug(modelformat) << "    transform" << joint.transform;
        qCDebug(modelformat) << "    rotationMin" << joint.rotationMin;
        qCDebug(modelformat) << "    rotationMax" << joint.rotationMax;
        qCDebug(modelformat) << "    inverseDefaultRotation" << joint.inverseDefaultRotation;
        qCDebug(modelformat) << "    inverseBindRotation" << joint.inverseBindRotation;
        qCDebug(modelformat) << "    bindTransform" << joint.bindTransform;
        qCDebug(modelformat) << "    name" << joint.name;
        qCDebug(modelformat) << "    shapePosition" << joint.shapePosition;
        qCDebug(modelformat) << "    shapeRotation" << joint.shapeRotation;
        qCDebug(modelformat) << "    shapeType" << joint.shapeType;
        qCDebug(modelformat) << "    isSkeletonJoint" << joint.isSkeletonJoint;
    }

    qCDebug(modelformat) << "\n";
}
