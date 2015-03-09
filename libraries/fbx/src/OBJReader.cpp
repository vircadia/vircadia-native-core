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


bool parseOBJGroup(OBJTokenizer &tokenizer, const QVariantHash& mapping, FBXGeometry &geometry) {
    FBXMesh &mesh = geometry.meshes[0];
    mesh.parts.append(FBXMeshPart());
    FBXMeshPart &meshPart = mesh.parts.last();
    bool sawG = false;
    bool result = true;

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
            // notice the order of z and y -- in OBJ files, up is the 3rd value
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            float z = std::stof(tokenizer.getDatum().data());
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            float y = std::stof(tokenizer.getDatum().data());
            if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) {
                break;
            }
            // the spec gets vague here.  might be w, might be a color... chop it off.
            tokenizer.skipLine();
            mesh.vertices.append(glm::vec3(x, y, z));
            mesh.colors.append(glm::vec3(1, 1, 1));
        } else if (token == "f") {
            // a face can have 3 or more vertices
            QVector<int> indices;
            while (true) {
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { goto done; }
                try {
                    int vertexIndex = std::stoi(tokenizer.getDatum().data());
                    // negative indexes count backward from the current end of the vertex list
                    vertexIndex = (vertexIndex >= 0 ? vertexIndex : mesh.vertices.count() + vertexIndex + 1);
                    // obj index is 1 based
                    assert(vertexIndex >= 1);
                    indices.append(vertexIndex - 1);
                }
                catch(const std::exception& e) {
                    // wasn't a number, but it back.
                    tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                    break;
                }
            }

            if (indices.count() == 3) {
                // flip these around (because of the y/z swap above) so our triangles face outward
                meshPart.triangleIndices.append(indices[0]);
                meshPart.triangleIndices.append(indices[2]);
                meshPart.triangleIndices.append(indices[1]);
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

    geometry.meshExtents.reset();
    geometry.meshes.append(FBXMesh());



    try {
        // call parseOBJGroup as long as it's returning true.  Each successful call will
        // add a new meshPart to the geometry's single mesh.
        bool success = true;
        while (success) {
            success = parseOBJGroup(tokenizer, mapping, geometry);
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
        geometry.joints[0].preRotation = glm::quat(0, 0, 0, 1);
        geometry.joints[0].rotation = glm::quat(0, 0, 0, 1);
        geometry.joints[0].postRotation = glm::quat(0, 0, 0, 1);
        // geometry.joints[0].postTransform = ;
        // geometry.joints[0].transform = ;
        geometry.joints[0].rotationMin = glm::vec3(0, 0, 0);
        geometry.joints[0].rotationMax = glm::vec3(0, 0, 0);
        geometry.joints[0].inverseDefaultRotation = glm::quat(0, 0, 0, 1);
        geometry.joints[0].inverseBindRotation = glm::quat(0, 0, 0, 1);
        // geometry.joints[0].bindTransform = ;
        geometry.joints[0].name = "OBJ";
        geometry.joints[0].shapePosition = glm::vec3(0, 0, 0);
        geometry.joints[0].shapeRotation = glm::quat(0, 0, 0, 1);
        geometry.joints[0].shapeType = SPHERE_SHAPE;
        geometry.joints[0].isSkeletonJoint = false;

        // add bogus normal data for this mesh
        mesh.normals.fill(glm::vec3(0,0,0), mesh.vertices.count());
        mesh.tangents.fill(glm::vec3(0,0,0), mesh.vertices.count());

        foreach (FBXMeshPart meshPart, mesh.parts) {
            int triCount = meshPart.triangleIndices.count() / 3;
            for (int i = 0; i < triCount; i++) {
                int p0Index = meshPart.triangleIndices[i*3];
                int p1Index = meshPart.triangleIndices[i*3+1];
                int p2Index = meshPart.triangleIndices[i*3+2];

                glm::vec3 p0 = mesh.vertices[p0Index];
                glm::vec3 p1 = mesh.vertices[p1Index];
                glm::vec3 p2 = mesh.vertices[p2Index];

                glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
                glm::vec3 t = glm::cross(p2 - p0, n);

                mesh.normals[p0Index] = n;
                mesh.normals[p1Index] = n;
                mesh.normals[p2Index] = n;

                mesh.tangents[p0Index] = t;
                mesh.tangents[p1Index] = t;
                mesh.tangents[p2Index] = t;
            }
        }
    }
    catch(const std::exception& e) {
        qDebug() << "something went wrong in OBJ reader";
    }

    return geometry;
}
