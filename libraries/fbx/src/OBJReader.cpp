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

#include "OBJReader.h"

#include <ctype.h>  // .obj files are not locale-specific. The C/ASCII charset applies.

#include <QtCore/QBuffer>
#include <QtCore/QIODevice>
#include <QtCore/QEventLoop>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include <shared/NsightHelpers.h>
#include <NetworkAccessManager.h>

#include "FBXReader.h"
#include "ModelFormatLogging.h"

QHash<QString, float> COMMENT_SCALE_HINTS = {{"This file uses centimeters as units", 1.0f / 100.0f},
                                             {"This file uses millimeters as units", 1.0f / 1000.0f}};

const QString SMART_DEFAULT_MATERIAL_NAME = "High Fidelity smart default material name";

namespace {
template<class T>
T& checked_at(QVector<T>& vector, int i) {
    if (i < 0 || i >= vector.size()) {
        throw std::out_of_range("index " + std::to_string(i) + "is out of range");
    }
    return vector[i];
}
}

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
    float uCoord = getFloat();
    float vCoord = 1.0f - getFloat();
    auto v = glm::vec2(uCoord, vCoord);
    while (isNextTokenFloat()) {
        // there can be a w, but we don't handle that
        nextToken();
    }
    return v;
}


void setMeshPartDefaults(FBXMeshPart& meshPart, QString materialID) {
    meshPart.materialID = materialID;
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

static bool replyOK(QNetworkReply* netReply, QUrl url) { // This will be reworked when we make things asynchronous
    return (netReply && netReply->isFinished() &&
            (url.toString().startsWith("file", Qt::CaseInsensitive) ? // file urls don't have http status codes
             netReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString().isEmpty() :
             (netReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200)));
}

bool OBJReader::isValidTexture(const QByteArray &filename) {
    if (_url.isEmpty()) {
        return false;
    }
    QUrl candidateUrl = _url.resolved(QUrl(filename));
    QNetworkReply *netReply = request(candidateUrl, true);
    bool isValid = replyOK(netReply, candidateUrl);
    if (netReply) {
        netReply->deleteLater();
    }
    return isValid;
}

void OBJReader::parseMaterialLibrary(QIODevice* device) {
    OBJTokenizer tokenizer(device);
    QString matName = SMART_DEFAULT_MATERIAL_NAME;
    OBJMaterial& currentMaterial = materials[matName];
    while (true) {
        switch (tokenizer.nextToken()) {
            case OBJTokenizer::COMMENT_TOKEN:
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJ Reader MTLLIB comment:" << tokenizer.getComment();
                #endif
                break;
            case OBJTokenizer::DATUM_TOKEN:
                break;
            default:
                materials[matName] = currentMaterial;
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJ Reader Last material shininess:" << currentMaterial.shininess << " opacity:" << currentMaterial.opacity << " diffuse color:" << currentMaterial.diffuseColor << " specular color:" << currentMaterial.specularColor << " diffuse texture:" << currentMaterial.diffuseTextureFilename << " specular texture:" << currentMaterial.specularTextureFilename;
                #endif
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
            #ifdef WANT_DEBUG
            qCDebug(modelformat) << "OBJ Reader Starting new material definition " << matName;
            #endif
            currentMaterial.diffuseTextureFilename = "";
        } else if (token == "Ns") {
            currentMaterial.shininess = tokenizer.getFloat();
        } else if ((token == "d") || (token == "Tr")) {
            currentMaterial.opacity = tokenizer.getFloat();
        } else if (token == "Ka") {
            #ifdef WANT_DEBUG
            qCDebug(modelformat) << "OBJ Reader Ignoring material Ka " << tokenizer.getVec3();
            #endif
        } else if (token == "Kd") {
            currentMaterial.diffuseColor = tokenizer.getVec3();
        } else if (token == "Ks") {
            currentMaterial.specularColor = tokenizer.getVec3();
        } else if ((token == "map_Kd") || (token == "map_Ks")) {
            QByteArray filename = QUrl(tokenizer.getLineAsDatum()).fileName().toUtf8();
            if (filename.endsWith(".tga")) {
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJ Reader WARNING: currently ignoring tga texture " << filename << " in " << _url;
                #endif
                break;
            }
            if (token == "map_Kd") {
                currentMaterial.diffuseTextureFilename = filename;
            } else {
                currentMaterial.specularTextureFilename = filename;
            }
        }
    }
}

QNetworkReply* OBJReader::request(QUrl& url, bool isTest) {
    if (!qApp) {
        return nullptr;
    }
    bool aboutToQuit{ false };
    auto connection = QObject::connect(qApp, &QCoreApplication::aboutToQuit, [&] {
        aboutToQuit = true;
    });
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest netRequest(url);
    netRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    QNetworkReply* netReply = isTest ? networkAccessManager.head(netRequest) : networkAccessManager.get(netRequest);
    if (!qApp || aboutToQuit) {
        netReply->deleteLater();
        return nullptr;
    }
    QEventLoop loop; // Create an event loop that will quit when we get the finished signal
    QObject::connect(netReply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();                    // Nothing is going to happen on this whole run thread until we get this
    static const int WAIT_TIMEOUT_MS = 500;
    while (qApp && !aboutToQuit && !netReply->isReadable()) {
        netReply->waitForReadyRead(WAIT_TIMEOUT_MS); // so we might as well block this thread waiting for the response, rather than
    }
    QObject::disconnect(connection);
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
        // we don't support separate objects in the same file, so treat "o" the same as "g".
        if (token == "g" || token == "o") {
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
        } else if (token == "mtllib" && !_url.isEmpty()) {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            QByteArray libraryName = tokenizer.getDatum();
            librariesSeen[libraryName] = true;
            // We'll read it later only if we actually need it.
        } else if (token == "usemtl") {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            QString nextName = tokenizer.getDatum();
            if (nextName != currentMaterialName) {
                currentMaterialName = nextName;
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJ Reader new current material:" << currentMaterialName;
                #endif
            }
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
    return result;
}


FBXGeometry* OBJReader::readOBJ(QByteArray& model, const QVariantHash& mapping, const QUrl& url) {
    PROFILE_RANGE_EX(modelformat, __FUNCTION__, 0xffff0000, nullptr);
    QBuffer buffer { &model };
    buffer.open(QIODevice::ReadOnly);

    FBXGeometry* geometryPtr = new FBXGeometry();
    FBXGeometry& geometry = *geometryPtr;
    OBJTokenizer tokenizer { &buffer };
    float scaleGuess = 1.0f;

    bool needsMaterialLibrary = false;

    _url = url;
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
        geometry.joints[0].translation = glm::vec3(0, 0, 0);
        geometry.joints[0].rotationMin = glm::vec3(0, 0, 0);
        geometry.joints[0].rotationMax = glm::vec3(0, 0, 0);
        geometry.joints[0].name = "OBJ";
        geometry.joints[0].isSkeletonJoint = true;

        geometry.jointIndices["x"] = 1;

        FBXCluster cluster;
        cluster.jointIndex = 0;
        cluster.inverseBindMatrix = glm::mat4(1, 0, 0, 0,
                                              0, 1, 0, 0,
                                              0, 0, 1, 0,
                                              0, 0, 0, 1);
        mesh.clusters.append(cluster);

        for (int i = 0, meshPartCount = 0; i < mesh.parts.count(); i++, meshPartCount++) {
            FBXMeshPart& meshPart = mesh.parts[i];
            FaceGroup faceGroup = faceGroups[meshPartCount];
            bool specifiesUV = false;
            foreach(OBJFace face, faceGroup) {
                glm::vec3 v0 = checked_at(vertices, face.vertexIndices[0]);
                glm::vec3 v1 = checked_at(vertices, face.vertexIndices[1]);
                glm::vec3 v2 = checked_at(vertices, face.vertexIndices[2]);
                meshPart.triangleIndices.append(mesh.vertices.count()); // not face.vertexIndices into vertices
                mesh.vertices << v0;
                meshPart.triangleIndices.append(mesh.vertices.count());
                mesh.vertices << v1;
                meshPart.triangleIndices.append(mesh.vertices.count());
                mesh.vertices << v2;

                glm::vec3 n0, n1, n2;
                if (face.normalIndices.count()) {
                    n0 = checked_at(normals, face.normalIndices[0]);
                    n1 = checked_at(normals, face.normalIndices[1]);
                    n2 = checked_at(normals, face.normalIndices[2]);
                } else { // generate normals from triangle plane if not provided
                    n0 = n1 = n2 = glm::cross(v1 - v0, v2 - v0);
                }
                mesh.normals << n0 << n1 << n2;
                if (face.textureUVIndices.count()) {
                    specifiesUV = true;
                    mesh.texCoords <<
                    checked_at(textureUVs, face.textureUVIndices[0]) <<
                    checked_at(textureUVs, face.textureUVIndices[1]) <<
                    checked_at(textureUVs, face.textureUVIndices[2]);
                } else {
                    glm::vec2 corner(0.0f, 1.0f);
                    mesh.texCoords << corner << corner << corner;
                }
            }
            // All the faces in the same group will have the same name and material.
            OBJFace leadFace = faceGroup[0];
            QString groupMaterialName = leadFace.materialName;
            if (groupMaterialName.isEmpty() && specifiesUV) {
                #ifdef WANT_DEBUG
                qCDebug(modelformat) << "OBJ Reader WARNING: " << url
                    << " needs a texture that isn't specified. Using default mechanism.";
                #endif
                groupMaterialName = SMART_DEFAULT_MATERIAL_NAME;
            }
            if (!groupMaterialName.isEmpty()) {
                OBJMaterial& material = materials[groupMaterialName];
                if (specifiesUV) {
                    material.userSpecifiesUV = true; // Note might not be true in a later usage.
                }
                if (specifiesUV || (0 != groupMaterialName.compare("none", Qt::CaseInsensitive))) {
                    // Blender has a convention that a material named "None" isn't really used (or defined).
                    material.used = true;
                    needsMaterialLibrary = groupMaterialName != SMART_DEFAULT_MATERIAL_NAME;
                }
                materials[groupMaterialName] = material;
                meshPart.materialID = groupMaterialName;
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

        FBXReader::buildModelMesh(mesh, url.toString());
        // fbxDebugDump(geometry);
    } catch(const std::exception& e) {
        qCDebug(modelformat) << "OBJ reader fail: " << e.what();
    }

    QString queryPart = _url.query();
    bool suppressMaterialsHack = queryPart.contains("hifiusemat"); // If this appears in query string, don't fetch mtl even if used.
    OBJMaterial& preDefinedMaterial = materials[SMART_DEFAULT_MATERIAL_NAME];
    if (suppressMaterialsHack) {
        needsMaterialLibrary = preDefinedMaterial.userSpecifiesUV = false; // I said it was a hack...
    }
    // Some .obj files use the convention that a group with uv coordinates that doesn't define a material, should use
    // a texture with the same basename as the .obj file.
    if (preDefinedMaterial.userSpecifiesUV && !url.isEmpty()) {
        QString filename = url.fileName();
        int extIndex = filename.lastIndexOf('.'); // by construction, this does not fail
        QString basename = filename.remove(extIndex + 1, sizeof("obj"));
        preDefinedMaterial.diffuseColor = glm::vec3(1.0f);
        QVector<QByteArray> extensions = { "jpg", "jpeg", "png", "tga" };
        QByteArray base = basename.toUtf8(), textName = "";
        qCDebug(modelformat) << "OBJ Reader looking for default texture of" << url;
        for (int i = 0; i < extensions.count(); i++) {
            QByteArray candidateString = base + extensions[i];
            if (isValidTexture(candidateString)) {
                textName = candidateString;
                break;
            }
        }

        if (!textName.isEmpty()) {
            #ifdef WANT_DEBUG
            qCDebug(modelformat) << "OBJ Reader found a default texture: " << textName;
            #endif
            preDefinedMaterial.diffuseTextureFilename = textName;
        }
        materials[SMART_DEFAULT_MATERIAL_NAME] = preDefinedMaterial;
    }
    if (needsMaterialLibrary) {
        foreach (QString libraryName, librariesSeen.keys()) {
            // Throw away any path part of libraryName, and merge against original url.
            QUrl libraryUrl = _url.resolved(QUrl(libraryName).fileName());
            qCDebug(modelformat) << "OBJ Reader material library" << libraryName << "used in" << _url;
            QNetworkReply* netReply = request(libraryUrl, false);
            if (replyOK(netReply, libraryUrl)) {
                parseMaterialLibrary(netReply);
            } else {
                qCDebug(modelformat) << "OBJ Reader WARNING:" << libraryName << "did not answer. Got"
                    << (!netReply ? "aborted" : netReply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
            }
            if (netReply) {
                netReply->deleteLater();
            }
        }
    }

    foreach (QString materialID, materials.keys()) {
       OBJMaterial& objMaterial = materials[materialID];
       if (!objMaterial.used) {
            continue;
        }
        geometry.materials[materialID] = FBXMaterial(objMaterial.diffuseColor,
                                                     objMaterial.specularColor,
                                                     glm::vec3(0.0f),
                                                     objMaterial.shininess,
                                                     objMaterial.opacity);
        FBXMaterial& fbxMaterial = geometry.materials[materialID];
        fbxMaterial.materialID = materialID;
        fbxMaterial._material = std::make_shared<model::Material>();
        model::MaterialPointer modelMaterial = fbxMaterial._material;

        if (!objMaterial.diffuseTextureFilename.isEmpty()) {
            fbxMaterial.albedoTexture.filename = objMaterial.diffuseTextureFilename;
        }

        modelMaterial->setEmissive(fbxMaterial.emissiveColor);
        modelMaterial->setAlbedo(fbxMaterial.diffuseColor);
        modelMaterial->setMetallic(glm::length(fbxMaterial.specularColor));
        modelMaterial->setRoughness(model::Material::shininessToRoughness(fbxMaterial.shininess));

        if (fbxMaterial.opacity <= 0.0f) {
            modelMaterial->setOpacity(1.0f);
        } else {
            modelMaterial->setOpacity(fbxMaterial.opacity);
        }
    }

    return geometryPtr;
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
   /*
            qCDebug(modelformat) << "        diffuseColor =" << meshPart.diffuseColor << "mat =" << meshPart._material->getDiffuse();
            qCDebug(modelformat) << "        specularColor =" << meshPart.specularColor << "mat =" << meshPart._material->getMetallic();
            qCDebug(modelformat) << "        emissiveColor =" << meshPart.emissiveColor << "mat =" << meshPart._material->getEmissive();
            qCDebug(modelformat) << "        emissiveParams =" << meshPart.emissiveParams;
            qCDebug(modelformat) << "        gloss =" << meshPart.shininess << "mat =" << meshPart._material->getRoughness();
            qCDebug(modelformat) << "        opacity =" << meshPart.opacity << "mat =" << meshPart._material->getOpacity();
            */
            qCDebug(modelformat) << "        materialID =" << meshPart.materialID;
      /*      qCDebug(modelformat) << "        diffuse texture =" << meshPart.diffuseTexture.filename;
            qCDebug(modelformat) << "        specular texture =" << meshPart.specularTexture.filename;
            */
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
        qCDebug(modelformat) << "    isSkeletonJoint" << joint.isSkeletonJoint;
    }

    qCDebug(modelformat) << "\n";
}
