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


class OBJTokenizer {
public:
    OBJTokenizer(QIODevice* device) : _device(device), _pushedBackToken(-1) { }
    enum SpecialToken {
        NO_TOKEN = -1,
        NO_PUSHBACKED_TOKEN = -1,
        DATUM_TOKEN = 0x100
    };
    int nextToken();
    const QByteArray& getDatum() const { return _datum; }
    bool isNextTokenFloat();
    void skipLine() { _device->readLine(); }
    void pushBackToken(int token) { _pushedBackToken = token; }
    void ungetChar(char ch) { _device->ungetChar(ch); }

private:
    QIODevice* _device;
    QByteArray _datum;
    int _pushedBackToken;
};


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
            case '#':
                _device->readLine(); // skip the comment
                break;

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

bool parseOBJGroup(OBJTokenizer &tokenizer, const QVariantHash& mapping,
                   FBXGeometry &geometry, QVector<glm::vec3>& faceNormals, QVector<int>& faceNormalIndexes) {
    FBXMesh &mesh = geometry.meshes[0];
    mesh.parts.append(FBXMeshPart());
    FBXMeshPart &meshPart = mesh.parts.last();
    bool sawG = false;
    bool result = true;

    meshPart.diffuseColor = glm::vec3(1, 1, 1);
    meshPart.specularColor = glm::vec3(1, 1, 1);
    meshPart.emissiveColor = glm::vec3(0, 0, 0);
    meshPart.emissiveParams = glm::vec2(0, 1);
    meshPart.shininess = 40;
    meshPart.opacity = 1;

    meshPart.materialID = QString("dontknow") + QString::number(mesh.parts.count());
    meshPart.opacity = 1.0;
    meshPart._material = model::MaterialPointer(new model::Material());
    meshPart._material->setDiffuse(glm::vec3(1.0, 1.0, 1.0));
    meshPart._material->setOpacity(1.0);
    meshPart._material->setSpecular(glm::vec3(1.0, 1.0, 1.0));
    meshPart._material->setShininess(96.0);
    meshPart._material->setEmissive(glm::vec3(0.0, 0.0, 0.0));

    while (true) {
        if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
            result = false;
            break;
        }
        QByteArray token = tokenizer.getDatum();
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
        } else if (token == "v") {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            float x = std::stof(tokenizer.getDatum().data());

            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            float y = std::stof(tokenizer.getDatum().data());

            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            float z = std::stof(tokenizer.getDatum().data());

            while (tokenizer.isNextTokenFloat()) {
                // the spec(s) get(s) vague here.  might be w, might be a color... chop it off.
                tokenizer.nextToken();
            }
            mesh.vertices.append(glm::vec3(x, y, z));
        } else if (token == "vn") {
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            float x = std::stof(tokenizer.getDatum().data());
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            float y = std::stof(tokenizer.getDatum().data());
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            float z = std::stof(tokenizer.getDatum().data());

            while (tokenizer.isNextTokenFloat()) {
                // the spec gets vague here.  might be w
                tokenizer.nextToken();
            }
            faceNormals.append(glm::vec3(x, y, z));
        } else if (token == "f") {
            // a face can have 3 or more vertices
            QVector<int> indices;
            QVector<int> normalIndices;
            while (true) {
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                    if (indices.count() == 0) {
                        goto done;
                    }
                    break;
                }
                // faces can be:
                //   vertex-index
                //   vertex-index/texture-index
                //   vertex-index/texture-index/surface-normal-index

                QByteArray token = tokenizer.getDatum();
                QList<QByteArray> parts = token.split('/');
                assert(parts.count() >= 1);
                assert(parts.count() <= 3);
                QByteArray vertIndexBA = parts[ 0 ];

                bool ok;
                int vertexIndex = vertIndexBA.toInt(&ok);
                if (!ok) {
                    // it wasn't #/#/#, put it back and exit this loop.
                    tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                    break;
                }

                // if (parts.count() > 1) {
                //     QByteArray textureIndexBA = parts[ 1 ];
                // }

                if (parts.count() > 2) {
                    QByteArray normalIndexBA = parts[ 2 ];
                    bool ok;
                    int normalIndex = normalIndexBA.toInt(&ok);
                    if (ok) {
                        normalIndices.append(normalIndex - 1);
                    }
                }

                // negative indexes count backward from the current end of the vertex list
                vertexIndex = (vertexIndex >= 0 ? vertexIndex : mesh.vertices.count() + vertexIndex + 1);
                // obj index is 1 based
                assert(vertexIndex >= 1);
                indices.append(vertexIndex - 1);
            }

            if (indices.count() == 3) {
                meshPart.triangleIndices.append(indices[0]);
                meshPart.triangleIndices.append(indices[1]);
                meshPart.triangleIndices.append(indices[2]);
                if (normalIndices.count() == 3) {
                    faceNormalIndexes.append(normalIndices[0]);
                    faceNormalIndexes.append(normalIndices[1]);
                    faceNormalIndexes.append(normalIndices[2]);
                } else {
                    // hmm.
                }
            } else if (indices.count() == 4) {
                meshPart.quadIndices << indices;
            } else {
                qDebug() << "no support for more than 4 vertices on a face in OBJ files";
            }
        } else {
            // something we don't (yet) care about
            qDebug() << "OBJ parser is skipping a line with" << token;
            tokenizer.skipLine();
        }
    }

 done:
    return result;
}


FBXGeometry extractOBJGeometry(const FBXNode& node, const QVariantHash& mapping) {
    FBXGeometry geometry;
    return geometry;
}


FBXGeometry readOBJ(const QByteArray& model, const QVariantHash& mapping) {
    QBuffer buffer(const_cast<QByteArray*>(&model));
    buffer.open(QIODevice::ReadOnly);
    return readOBJ(&buffer, mapping);
}


FBXGeometry readOBJ(QIODevice* device, const QVariantHash& mapping) {
    FBXGeometry geometry;
    OBJTokenizer tokenizer(device);
    QVector<int> faceNormalIndexes;
    QVector<glm::vec3> faceNormals;

    faceNormalIndexes.clear();

    geometry.meshExtents.reset();
    geometry.meshes.append(FBXMesh());



    try {
        // call parseOBJGroup as long as it's returning true.  Each successful call will
        // add a new meshPart to the geometry's single mesh.
        bool success = true;
        while (success) {
            success = parseOBJGroup(tokenizer, mapping, geometry, faceNormals, faceNormalIndexes);
        }

        FBXMesh &mesh = geometry.meshes[0];

        mesh.meshExtents.reset();
        foreach (const glm::vec3& vertex, mesh.vertices) {
            mesh.meshExtents.addPoint(vertex);
            geometry.meshExtents.addPoint(vertex);
        }

        geometry.joints.resize(1);
        geometry.joints[0].isFree = false;
        // geometry.joints[0].freeLineage;
        geometry.joints[0].parentIndex = -1;
        geometry.joints[0].distanceToParent = 0;
        geometry.joints[0].boneRadius = 0;
        geometry.joints[0].translation = glm::vec3(0, 0, 0);
        // geometry.joints[0].preTransform = ;
        geometry.joints[0].preRotation = glm::quat(1, 0, 0, 0);
        geometry.joints[0].rotation = glm::quat(1, 0, 0, 0);
        geometry.joints[0].postRotation = glm::quat(1, 0, 0, 0);
        // geometry.joints[0].postTransform = ;
        // geometry.joints[0].transform = ;
        geometry.joints[0].rotationMin = glm::vec3(0, 0, 0);
        geometry.joints[0].rotationMax = glm::vec3(0, 0, 0);
        geometry.joints[0].inverseDefaultRotation = glm::quat(1, 0, 0, 0);
        geometry.joints[0].inverseBindRotation = glm::quat(1, 0, 0, 0);
        // geometry.joints[0].bindTransform = ;
        geometry.joints[0].name = "OBJ";
        geometry.joints[0].shapePosition = glm::vec3(0, 0, 0);
        geometry.joints[0].shapeRotation = glm::quat(1, 0, 0, 0);
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

        // The OBJ format has normals for faces.  The FBXGeometry structure has normals for points.
        // run through all the faces, look-up (or determine) a normal and set the normal for the points
        // that make up each face.
        QVector<glm::vec3> pointNormalsSums;
        QVector<int> pointNormalsCounts;

        mesh.normals.fill(glm::vec3(0,0,0), mesh.vertices.count());
        pointNormalsSums.fill(glm::vec3(0,0,0), mesh.vertices.count());
        pointNormalsCounts.fill(0, mesh.vertices.count());

        foreach (FBXMeshPart meshPart, mesh.parts) {
            int triCount = meshPart.triangleIndices.count() / 3;
            for (int i = 0; i < triCount; i++) {
                int p0Index = meshPart.triangleIndices[i*3];
                int p1Index = meshPart.triangleIndices[i*3+1];
                int p2Index = meshPart.triangleIndices[i*3+2];

                assert(p0Index < mesh.vertices.count());
                assert(p1Index < mesh.vertices.count());
                assert(p2Index < mesh.vertices.count());

                glm::vec3 n0, n1, n2;
                if (i < faceNormalIndexes.count()) {
                    int n0Index = faceNormalIndexes[i*3];
                    int n1Index = faceNormalIndexes[i*3+1];
                    int n2Index = faceNormalIndexes[i*3+2];
                    n0 = faceNormals[n0Index];
                    n1 = faceNormals[n1Index];
                    n2 = faceNormals[n2Index];
                } else {
                    // We didn't read normals, add bogus normal data for this face
                    glm::vec3 p0 = mesh.vertices[p0Index];
                    glm::vec3 p1 = mesh.vertices[p1Index];
                    glm::vec3 p2 = mesh.vertices[p2Index];
                    n0 = glm::cross(p1 - p0, p2 - p0);
                    n1 = n0;
                    n2 = n0;
                }

                // we sum up the normal for each point and then divide by the count to get an average
                pointNormalsSums[p0Index] += n0;
                pointNormalsSums[p1Index] += n1;
                pointNormalsSums[p2Index] += n2;
                pointNormalsCounts[p0Index]++;
                pointNormalsCounts[p1Index]++;
                pointNormalsCounts[p2Index]++;
            }

            int vertCount = mesh.vertices.count();
            for (int i = 0; i < vertCount; i++) {
                if (pointNormalsCounts[i] > 0) {
                    mesh.normals[i] = glm::normalize(pointNormalsSums[i] / (float)(pointNormalsCounts[i]));
                }
            }

            // XXX do same normal calculation for quadCount
        }
    }
    catch(const std::exception& e) {
        qDebug() << "something went wrong in OBJ reader";
    }

    return geometry;
}



void fbxDebugDump(const FBXGeometry& fbxgeo) {
    qDebug() << "---------------- fbxGeometry ----------------";
    qDebug() << "  hasSkeletonJoints =" << fbxgeo.hasSkeletonJoints;
    qDebug() << "  offset =" << fbxgeo.offset;
    qDebug() << "  attachments.count() = " << fbxgeo.attachments.count();
    qDebug() << "  meshes.count() =" << fbxgeo.meshes.count();
    foreach (FBXMesh mesh, fbxgeo.meshes) {
        qDebug() << "    vertices.count() =" << mesh.vertices.count();
        qDebug() << "    normals.count() =" << mesh.normals.count();
        if (mesh.normals.count() == mesh.vertices.count()) {
            for (int i = 0; i < mesh.normals.count(); i++) {
                qDebug() << "        " << mesh.vertices[ i ] << mesh.normals[ i ];
            }
        }
        qDebug() << "    tangents.count() =" << mesh.tangents.count();
        qDebug() << "    colors.count() =" << mesh.colors.count();
        qDebug() << "    texCoords.count() =" << mesh.texCoords.count();
        qDebug() << "    texCoords1.count() =" << mesh.texCoords1.count();
        qDebug() << "    clusterIndices.count() =" << mesh.clusterIndices.count();
        qDebug() << "    clusterWeights.count() =" << mesh.clusterWeights.count();
        qDebug() << "    meshExtents =" << mesh.meshExtents;
        qDebug() << "    modelTransform =" << mesh.modelTransform;
        qDebug() << "    parts.count() =" << mesh.parts.count();
        foreach (FBXMeshPart meshPart, mesh.parts) {
            qDebug() << "        quadIndices.count() =" << meshPart.quadIndices.count();
            qDebug() << "        triangleIndices.count() =" << meshPart.triangleIndices.count();
            qDebug() << "        diffuseColor =" << meshPart.diffuseColor;
            qDebug() << "        specularColor =" << meshPart.specularColor;
            qDebug() << "        emissiveColor =" << meshPart.emissiveColor;
            qDebug() << "        emissiveParams =" << meshPart.emissiveParams;
            qDebug() << "        shininess =" << meshPart.shininess;
            qDebug() << "        opacity =" << meshPart.opacity;
            qDebug() << "        materialID =" << meshPart.materialID;
        }
        qDebug() << "    clusters.count() =" << mesh.clusters.count();
        foreach (FBXCluster cluster, mesh.clusters) {
            qDebug() << "        jointIndex =" << cluster.jointIndex;
            qDebug() << "        inverseBindMatrix =" << cluster.inverseBindMatrix;
        }
    }

    qDebug() << "  jointIndices =" << fbxgeo.jointIndices;
    qDebug() << "  joints.count() =" << fbxgeo.joints.count();

    foreach (FBXJoint joint, fbxgeo.joints) {
        qDebug() << "    isFree =" << joint.isFree;
        qDebug() << "    freeLineage" << joint.freeLineage;
        qDebug() << "    parentIndex" << joint.parentIndex;
        qDebug() << "    distanceToParent" << joint.distanceToParent;
        qDebug() << "    boneRadius" << joint.boneRadius;
        qDebug() << "    translation" << joint.translation;
        qDebug() << "    preTransform" << joint.preTransform;
        qDebug() << "    preRotation" << joint.preRotation;
        qDebug() << "    rotation" << joint.rotation;
        qDebug() << "    postRotation" << joint.postRotation;
        qDebug() << "    postTransform" << joint.postTransform;
        qDebug() << "    transform" << joint.transform;
        qDebug() << "    rotationMin" << joint.rotationMin;
        qDebug() << "    rotationMax" << joint.rotationMax;
        qDebug() << "    inverseDefaultRotation" << joint.inverseDefaultRotation;
        qDebug() << "    inverseBindRotation" << joint.inverseBindRotation;
        qDebug() << "    bindTransform" << joint.bindTransform;
        qDebug() << "    name" << joint.name;
        qDebug() << "    shapePosition" << joint.shapePosition;
        qDebug() << "    shapeRotation" << joint.shapeRotation;
        qDebug() << "    shapeType" << joint.shapeType;
        qDebug() << "    isSkeletonJoint" << joint.isSkeletonJoint;
    }

    qDebug() << "\n";
}
