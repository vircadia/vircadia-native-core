


// http://en.wikipedia.org/wiki/Wavefront_.obj_file


#include <QBuffer>
#include <QIODevice>

#include "FBXReader.h"
#include "OBJReader.h"
#include "Shape.h"


class OBJTokenizer {
public:
    OBJTokenizer(QIODevice* device) : _device(device), _pushedBackToken(-1) { }
    enum SpecialToken { DATUM_TOKEN = 0x100 };
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
    if (_pushedBackToken != -1) {
        int token = _pushedBackToken;
        _pushedBackToken = -1;
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
    return -1;
}



bool parseOBJMesh(OBJTokenizer &tokenizer, const QVariantHash& mapping, FBXGeometry &geometry, int& indexStart) {
    FBXMesh mesh;
    FBXMeshPart meshPart;
    bool sawG = false;
    bool meshHasData = true;
    bool result = true;


    // QString materialID;
    // model::MaterialPointer _material;

    meshPart.materialID = "dontknow";
    // meshPart._material = { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(), 96.0f, 1.0f };
    meshPart._material = model::MaterialPointer(new model::Material());
    meshPart.opacity = 1.0;

    // material._material->setEmissive(material.emissive); 
    // material._material->setDiffuse(material.diffuse); 
    // material._material->setSpecular(material.specular); 
    // material._material->setShininess(material.shininess); 
    // material._material->setOpacity(material.opacity); 


    try { // XXX move this out/up
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
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                QByteArray groupName = tokenizer.getDatum();
                meshPart.materialID = groupName;
                meshHasData = true;
            } else if (token == "v") {
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                float x = std::stof(tokenizer.getDatum().data());
                // notice the order of z and y -- in OBJ files, up is the 3rd value
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                float z = std::stof(tokenizer.getDatum().data());
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }
                float y = std::stof(tokenizer.getDatum().data());
                if (tokenizer.nextToken() != OBJTokenizer::DATUM_TOKEN) { break; }

                // the spec gets vague here.  might be w, might be a color... chop it off.
                tokenizer.skipLine();

                mesh.vertices.append(glm::vec3(x, y, z));
                mesh.colors.append(glm::vec3(1, 1, 1));
                meshHasData = true;
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
                        vertexIndex = vertexIndex - 1 - indexStart;
                        assert(vertexIndex >= 0);
                        indices.append(vertexIndex);
                    }
                    catch(const std::exception& e) {
                        // wasn't a number, but it back.
                        tokenizer.pushBackToken(OBJTokenizer::DATUM_TOKEN);
                        break;
                    }
                }

                if (indices.count() == 3) {
                    meshPart.triangleIndices << indices;
                } else if (indices.count() == 4) {
                    meshPart.quadIndices << indices;
                } else {
                    qDebug() << "no support for more than 4 vertices on a face in OBJ files";
                }
                meshHasData = true;
            } else {
                // something we don't (yet) care about
                //   vp
                //   mtllib
                //   usemtl
                //   s 1, s off

                qDebug() << "skipping line with" << token;
                tokenizer.skipLine();
            }
        }
    }
    catch(const std::exception& e) {
        // something went wrong, stop here.
        qDebug() << "something went wrong in obj reader";
        return false;
    }

 done:

#if 0
    // add bogus normal data for this mesh
    // mesh.normals.resize(mesh.vertices.count());
    mesh.normals.fill(glm::vec3(0,0,0), mesh.vertices.count());
    mesh.tangents.fill(glm::vec3(0,0,0), mesh.vertices.count());
    int triCount = meshPart.triangleIndices.count() / 3;
    for (int i = 0; i < triCount; i++) {
        int p0Index = meshPart.triangleIndices[ i*3 ];
        int p1Index = meshPart.triangleIndices[ i*3+1 ];
        int p2Index = meshPart.triangleIndices[ i*3+2 ];

        glm::vec3 p0 = mesh.vertices[ p0Index ];
        glm::vec3 p1 = mesh.vertices[ p1Index ];
        glm::vec3 p2 = mesh.vertices[ p2Index ];

        glm::vec3 n = glm::cross(p2 - p0, p1 - p0);
        glm::vec3 t = glm::cross(p2 - p0, n);

        mesh.normals[ p0Index ] = n;
        mesh.normals[ p1Index ] = n;
        mesh.normals[ p2Index ] = n;

        mesh.tangents[ p0Index ] = t;
        mesh.tangents[ p1Index ] = t;
        mesh.tangents[ p2Index ] = t;
    }
#endif


    if (! meshHasData)
        return result;


    mesh.meshExtents.reset();
    foreach (const glm::vec3& vertex, mesh.vertices) {
        mesh.meshExtents.addPoint(vertex);
        geometry.meshExtents.addPoint(vertex);
    }

    mesh.parts.append(meshPart);
    geometry.meshes.append(mesh);


    geometry.joints.resize(1);
    geometry.joints[ 0 ].isFree = false;
    // geometry.joints[ 0 ].freeLineage;
    geometry.joints[ 0 ].parentIndex = -1;
    geometry.joints[ 0 ].distanceToParent = 0;
    geometry.joints[ 0 ].boneRadius = 0;
    geometry.joints[ 0 ].translation = glm::vec3(0, 0, 0);
    // geometry.joints[ 0 ].preTransform = ;
    geometry.joints[ 0 ].preRotation = glm::quat(0, 0, 0, 1);
    geometry.joints[ 0 ].rotation = glm::quat(0, 0, 0, 1);
    geometry.joints[ 0 ].postRotation = glm::quat(0, 0, 0, 1);
    // geometry.joints[ 0 ].postTransform = ;
    // geometry.joints[ 0 ].transform = ;
    geometry.joints[ 0 ].rotationMin = glm::vec3(0, 0, 0);
    geometry.joints[ 0 ].rotationMax = glm::vec3(0, 0, 0);
    geometry.joints[ 0 ].inverseDefaultRotation = glm::quat(0, 0, 0, 1);
    geometry.joints[ 0 ].inverseBindRotation = glm::quat(0, 0, 0, 1);
    // geometry.joints[ 0 ].bindTransform = ;
    geometry.joints[ 0 ].name = "OBJ";
    geometry.joints[ 0 ].shapePosition = glm::vec3(0, 0, 0);
    geometry.joints[ 0 ].shapeRotation = glm::quat(0, 0, 0, 1);
    geometry.joints[ 0 ].shapeType = SPHERE_SHAPE;
    geometry.joints[ 0 ].isSkeletonJoint = false;

    indexStart += mesh.vertices.count();

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
    int indexStart = 0;

    geometry.meshExtents.reset();

    while (true) {
        if (!parseOBJMesh(tokenizer, mapping, geometry, indexStart)) {
            break;
        }

        qDebug() << "indexStart is" << indexStart;
    }

    return geometry;
}
