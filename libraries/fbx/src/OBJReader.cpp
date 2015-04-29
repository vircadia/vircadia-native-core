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

#include "FBXReader.h"
#include "OBJReader.h"
#include "Shape.h"
#include "ModelFormatLogging.h"


QHash<QString, float> COMMENT_SCALE_HINTS = {{"This file uses centimeters as units", 1.0f / 100.0f},
                                             {"This file uses millimeters as units", 1.0f / 1000.0f}};


OBJTokenizer::OBJTokenizer(QIODevice* device) : _device(device), _pushedBackToken(-1) {
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
    auto v = glm::vec3(getFloat(), getFloat(), getFloat());  // N.B.: getFloat() has side-effect
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
    meshPart._material = model::MaterialPointer(new model::Material());
    meshPart._material->setDiffuse(glm::vec3(1.0, 1.0, 1.0));
    meshPart._material->setOpacity(1.0);
    meshPart._material->setSpecular(glm::vec3(1.0, 1.0, 1.0));
    meshPart._material->setShininess(96.0);
    meshPart._material->setEmissive(glm::vec3(0.0, 0.0, 0.0));
}

// OBJFace
bool OBJFace::add(QByteArray vertexIndex, QByteArray textureIndex, QByteArray normalIndex) {
    bool ok;
    int index = vertexIndex.toInt(&ok);
    if (!ok) {
        return false;
    }
    vertexIndices.append(index - 1);
    if (textureIndex != nullptr) {
        index = textureIndex.toInt(&ok);
        if (!ok) {
            return false;
        }
        textureUVIndices.append(index - 1);
    }
    if (normalIndex != nullptr) {
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
    if (vertexIndices.count() == 3) {
        newFaces.append(*this);
    } else {
        for (int i = 1; i < vertexIndices.count() - 1; i++) {
            OBJFace newFace; // FIXME: also copy materialName, groupName
            newFace.addFrom(this, 0);
            newFace.addFrom(this, i);
            newFace.addFrom(this, i + 1);
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

bool OBJReader::parseOBJGroup(OBJTokenizer& tokenizer, const QVariantHash& mapping, FBXGeometry& geometry, float& scaleGuess) {
    FaceGroup faces;
    FBXMesh& mesh = geometry.meshes[0];
    mesh.parts.append(FBXMeshPart());
    FBXMeshPart& meshPart = mesh.parts.last();
    bool sawG = false;
    bool result = true;
    int originalFaceCountForDebugging = 0;

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
            meshPart.materialID = groupName;
            //qCDebug(modelformat) << "new group:" << groupName;
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
                if (!std::isdigit(token[0])) { // Tokenizer treats line endings as whitespace. Non-digit indicates done;
                    tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                    break;
                }
                QList<QByteArray> parts = token.split('/');
                assert(parts.count() >= 1);
                assert(parts.count() <= 3);
                // FIXME: if we want to handle negative indices, it has to be done here.
                face.add(parts[0], (parts.count() > 1) ? parts[1] : nullptr, (parts.count() > 2) ? parts[2] : nullptr);
                // FIXME: preserve current name, material and such
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
    }
    faceGroups.append(faces); // We're done with this group. Add the faces.
    //qCDebug(modelformat) << "end group:" << meshPart.materialID << " original faces:" << originalFaceCountForDebugging << " triangles:" << faces.count() << " keep going:" << result;
    return result;
}


FBXGeometry OBJReader::readOBJ(const QByteArray& model, const QVariantHash& mapping) {
    QBuffer buffer(const_cast<QByteArray*>(&model));
    buffer.open(QIODevice::ReadOnly);
    return readOBJ(&buffer, mapping);
}


FBXGeometry OBJReader::readOBJ(QIODevice* device, const QVariantHash& mapping) {
    FBXGeometry geometry;
    OBJTokenizer tokenizer(device);
    float scaleGuess = 1.0f;

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
        
        int meshPartCount = 0;
        for (int i = 0; i < mesh.parts.count(); i++) {
            FBXMeshPart& meshPart = mesh.parts[i];
            //qCDebug(modelformat) << "part:" << meshPartCount << " faces:" << faceGroups[meshPartCount].count() << "triangle indices will start with:" << mesh.vertices.count();
            foreach(OBJFace face, faceGroups[meshPartCount]) {
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
                }
            }
            meshPartCount++;
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
        //fbxDebugDump(geometry);
    }
    catch(const std::exception& e) {
        qCDebug(modelformat) << "something went wrong in OBJ reader: " << e.what();
    }

    return geometry;
}



void OBJReader::fbxDebugDump(const FBXGeometry& fbxgeo) {
    qCDebug(modelformat) << "---------------- fbxGeometry ----------------";
    qCDebug(modelformat) << "  hasSkeletonJoints =" << fbxgeo.hasSkeletonJoints;
    qCDebug(modelformat) << "  offset =" << fbxgeo.offset;
    qCDebug(modelformat) << "  attachments.count() = " << fbxgeo.attachments.count();
    qCDebug(modelformat) << "  meshes.count() =" << fbxgeo.meshes.count();
    foreach (FBXMesh mesh, fbxgeo.meshes) {
        qCDebug(modelformat) << "    vertices.count() =" << mesh.vertices.count();
        qCDebug(modelformat) << "    normals.count() =" << mesh.normals.count();
        if (mesh.normals.count() == mesh.vertices.count()) {
            for (int i = 0; i < mesh.normals.count(); i++) {
                qCDebug(modelformat) << "        " << mesh.vertices[ i ] << mesh.normals[ i ];
            }
        }
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
            qCDebug(modelformat) << "        diffuseColor =" << meshPart.diffuseColor;
            qCDebug(modelformat) << "        specularColor =" << meshPart.specularColor;
            qCDebug(modelformat) << "        emissiveColor =" << meshPart.emissiveColor;
            qCDebug(modelformat) << "        emissiveParams =" << meshPart.emissiveParams;
            qCDebug(modelformat) << "        shininess =" << meshPart.shininess;
            qCDebug(modelformat) << "        opacity =" << meshPart.opacity;
            qCDebug(modelformat) << "        materialID =" << meshPart.materialID;
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
