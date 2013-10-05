//
//  FBXReader.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <QBuffer>
#include <QDataStream>
#include <QIODevice>
#include <QtDebug>
#include <QtEndian>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include "FBXReader.h"

using namespace std;

template<class T> QVariant readArray(QDataStream& in) {
    quint32 arrayLength;
    quint32 encoding;
    quint32 compressedLength;
    
    in >> arrayLength;
    in >> encoding;
    in >> compressedLength;
    
    QVector<T> values;
    const int DEFLATE_ENCODING = 1;
    if (encoding == DEFLATE_ENCODING) {
        // preface encoded data with uncompressed length
        QByteArray compressed(sizeof(quint32) + compressedLength, 0);
        *((quint32*)compressed.data()) = qToBigEndian<quint32>(arrayLength * sizeof(T));
        in.readRawData(compressed.data() + sizeof(quint32), compressedLength);
        QByteArray uncompressed = qUncompress(compressed);
        QDataStream uncompressedIn(uncompressed);
        uncompressedIn.setByteOrder(QDataStream::LittleEndian);
        uncompressedIn.setVersion(QDataStream::Qt_4_5); // for single/double precision switch
        for (int i = 0; i < arrayLength; i++) {
            T value;
            uncompressedIn >> value;
            values.append(value);
        }
    } else {
        for (int i = 0; i < arrayLength; i++) {
            T value;
            in >> value;
            values.append(value);
        }
    }
    return QVariant::fromValue(values);
}

QVariant parseFBXProperty(QDataStream& in) {
    char ch;
    in.device()->getChar(&ch);
    switch (ch) {
        case 'Y': {
            qint16 value;
            in >> value;
            return QVariant::fromValue(value);   
        }
        case 'C': {
            bool value;
            in >> value;
            return QVariant::fromValue(value);
        }
        case 'I': {
            qint32 value;
            in >> value;
            return QVariant::fromValue(value);
        }
        case 'F': {
            float value;
            in >> value;
            return QVariant::fromValue(value);
        }
        case 'D': {
            double value;
            in >> value;
            return QVariant::fromValue(value);
        }
        case 'L': {
            qint64 value;
            in >> value;
            return QVariant::fromValue(value);
        }
        case 'f': {
            return readArray<float>(in);
        }
        case 'd': {
            return readArray<double>(in);
        }
        case 'l': {
            return readArray<qint64>(in);
        }
        case 'i': {
            return readArray<qint32>(in);
        }
        case 'b': {
            return readArray<bool>(in);
        }
        case 'S':
        case 'R': {
            quint32 length;
            in >> length;
            return QVariant::fromValue(in.device()->read(length));
        }
        default:
            throw QString("Unknown property type: ") + ch;
    }
}

FBXNode parseFBXNode(QDataStream& in) {
    quint32 endOffset;
    quint32 propertyCount;
    quint32 propertyListLength;
    quint8 nameLength;
    
    in >> endOffset;
    in >> propertyCount;
    in >> propertyListLength;
    in >> nameLength;
    
    FBXNode node;
    const int MIN_VALID_OFFSET = 40;
    if (endOffset < MIN_VALID_OFFSET || nameLength == 0) {
        // use a null name to indicate a null node
        return node;
    }
    node.name = in.device()->read(nameLength);
    
    for (int i = 0; i < propertyCount; i++) {
        node.properties.append(parseFBXProperty(in));    
    }
    
    while (endOffset > in.device()->pos()) {
        FBXNode child = parseFBXNode(in);
        if (child.name.isNull()) {
            return node;
            
        } else {
            node.children.append(child);
        }
    }
    
    return node;
}

FBXNode parseFBX(QIODevice* device) {
    QDataStream in(device);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setVersion(QDataStream::Qt_4_5); // for single/double precision switch
    
    // see http://code.blender.org/index.php/2013/08/fbx-binary-file-format-specification/ for an explanation
    // of the FBX format
    
    // verify the prolog
    const QByteArray EXPECTED_PROLOG = "Kaydara FBX Binary  ";
    if (device->read(EXPECTED_PROLOG.size()) != EXPECTED_PROLOG) {
        throw QString("Invalid header.");
    }
    
    // skip the rest of the header
    const int HEADER_SIZE = 27;
    in.skipRawData(HEADER_SIZE - EXPECTED_PROLOG.size());
    
    // parse the top-level node
    FBXNode top;
    while (device->bytesAvailable()) {
        FBXNode next = parseFBXNode(in);
        if (next.name.isNull()) {
            return top;
            
        } else {
            top.children.append(next);
        }
    }
    
    return top;
}

QVariantHash parseMapping(QIODevice* device) {
    QVariantHash properties;
    
    QByteArray line;
    while (!(line = device->readLine()).isEmpty()) {
        if ((line = line.trimmed()).startsWith('#')) {
            continue; // comment
        }
        QList<QByteArray> sections = line.split('=');
        if (sections.size() < 2) {
            continue;
        }
        QByteArray name = sections.at(0).trimmed();
        if (sections.size() == 2) {
            properties.insert(name, sections.at(1).trimmed());
        
        } else if (sections.size() == 3) {
            QVariantHash heading = properties.value(name).toHash();
            heading.insert(sections.at(1).trimmed(), sections.at(2).trimmed());
            properties.insert(name, heading);
            
        } else if (sections.size() >= 4) {
            QVariantHash heading = properties.value(name).toHash();
            QVariantList contents;
            for (int i = 2; i < sections.size(); i++) {
                contents.append(sections.at(i).trimmed());
            }
            heading.insertMulti(sections.at(1).trimmed(), contents);
            properties.insert(name, heading);
        }
    }
    
    return properties;
}

QVector<glm::vec3> createVec3Vector(const QVector<double>& doubleVector) {
    QVector<glm::vec3> values;
    for (const double* it = doubleVector.constData(), *end = it + doubleVector.size(); it != end; ) {
        float x = *it++;
        float y = *it++;
        float z = *it++;
        values.append(glm::vec3(x, y, z));
    }
    return values;
}

QVector<glm::vec2> createVec2Vector(const QVector<double>& doubleVector) {
    QVector<glm::vec2> values;
    for (const double* it = doubleVector.constData(), *end = it + doubleVector.size(); it != end; ) {
        float s = *it++;
        float t = *it++;
        values.append(glm::vec2(s, -t));
    }
    return values;
}

glm::mat4 createMat4(const QVector<double>& doubleVector) {
    return glm::mat4(doubleVector.at(0), doubleVector.at(1), doubleVector.at(2), doubleVector.at(3),
        doubleVector.at(4), doubleVector.at(5), doubleVector.at(6), doubleVector.at(7),
        doubleVector.at(8), doubleVector.at(9), doubleVector.at(10), doubleVector.at(11),
        doubleVector.at(12), doubleVector.at(13), doubleVector.at(14), doubleVector.at(15));
}

const char* FACESHIFT_BLENDSHAPES[] = {
    "EyeBlink_L",
    "EyeBlink_R",
    "EyeSquint_L",
    "EyeSquint_R",
    "EyeDown_L",
    "EyeDown_R",
    "EyeIn_L",
    "EyeIn_R",
    "EyeOpen_L",
    "EyeOpen_R",
    "EyeOut_L",
    "EyeOut_R",
    "EyeUp_L",
    "EyeUp_R",
    "BrowsD_L",
    "BrowsD_R",
    "BrowsU_C",
    "BrowsU_L",
    "BrowsU_R",
    "JawFwd",
    "JawLeft",
    "JawOpen",
    "JawChew",
    "JawRight",
    "MouthLeft",
    "MouthRight",
    "MouthFrown_L",
    "MouthFrown_R",
    "MouthSmile_L",
    "MouthSmile_R",
    "MouthDimple_L",
    "MouthDimple_R",
    "LipsStretch_L",
    "LipsStretch_R",
    "LipsUpperClose",
    "LipsLowerClose",
    "LipsUpperUp",
    "LipsLowerDown",
    "LipsUpperOpen",
    "LipsLowerOpen",
    "LipsFunnel",
    "LipsPucker",
    "ChinLowerRaise",
    "ChinUpperRaise",
    "Sneer",
    "Puff",
    "CheekSquint_L",
    "CheekSquint_R",
    ""
};

class Transform {
public:
    bool inheritScale;
    glm::mat4 withScale;
    glm::mat4 withoutScale;
};

glm::mat4 getGlobalTransform(const QMultiHash<qint64, qint64>& parentMap, const QHash<qint64, Transform>& localTransforms,
    qint64 nodeID, bool forceScale = true) {
    
    glm::mat4 globalTransform;
    bool useScale = true;
    while (nodeID != 0) {
        const Transform& localTransform = localTransforms.value(nodeID);
        globalTransform = (useScale ? localTransform.withScale : localTransform.withoutScale) * globalTransform;
        useScale = (useScale && localTransform.inheritScale) || forceScale;
        
        QList<qint64> parentIDs = parentMap.values(nodeID);
        nodeID = 0;
        foreach (qint64 parentID, parentIDs) {
            if (localTransforms.contains(parentID)) {
                nodeID = parentID;
                break;
            }
        }
    }
    
    return globalTransform;
}

class ExtractedBlendshape {
public:
    qint64 id;
    FBXBlendshape blendshape;
};

void printNode(const FBXNode& node, int indent) {
    QByteArray spaces(indent, ' ');
    qDebug("%s%s: ", spaces.data(), node.name.data());
    foreach (const QVariant& property, node.properties) {
        qDebug() << property;
    }
    qDebug() << "\n";
    foreach (const FBXNode& child, node.children) {
        printNode(child, indent + 1);
    }
}

class Material {
public:
    glm::vec3 diffuse;
    glm::vec3 specular;
    float shininess;
};

FBXGeometry extractFBXGeometry(const FBXNode& node, const QVariantHash& mapping) {
    QHash<qint64, FBXMesh> meshes;
    QVector<ExtractedBlendshape> blendshapes;
    QMultiHash<qint64, qint64> parentMap;
    QMultiHash<qint64, qint64> childMap;
    QHash<qint64, Transform> localTransforms;
    QHash<qint64, glm::mat4> transformLinkMatrices;
    QHash<qint64, QByteArray> textureFilenames;
    QHash<qint64, Material> materials;
    QHash<qint64, qint64> diffuseTextures;
    QHash<qint64, qint64> bumpTextures;
    
    QVariantHash joints = mapping.value("joint").toHash();
    QByteArray jointEyeLeftName = joints.value("jointEyeLeft", "jointEyeLeft").toByteArray();
    QByteArray jointEyeRightName = joints.value("jointEyeRight", "jointEyeRight").toByteArray();
    QByteArray jointNeckName = joints.value("jointNeck", "jointNeck").toByteArray();
    qint64 jointEyeLeftID = 0;
    qint64 jointEyeRightID = 0;
    qint64 jointNeckID = 0;
    
    QVariantHash blendshapeMappings = mapping.value("bs").toHash();
    QHash<QByteArray, QPair<int, float> > blendshapeIndices;
    for (int i = 0;; i++) {
        QByteArray blendshapeName = FACESHIFT_BLENDSHAPES[i];
        if (blendshapeName.isEmpty()) {
            break;
        }
        QList<QVariant> mappings = blendshapeMappings.values(blendshapeName);
        if (mappings.isEmpty()) {
            blendshapeIndices.insert("ExpressionBlendshapes." + blendshapeName, QPair<int, float>(i, 1.0f));
        } else {
            foreach (const QVariant& mapping, mappings) {
                QVariantList blendshapeMapping = mapping.toList();
                blendshapeIndices.insert(blendshapeMapping.at(0).toByteArray(),
                   QPair<int, float>(i, blendshapeMapping.at(1).toFloat()));
            }
        }
    }
    QHash<qint64, QPair<int, float> > blendshapeChannelIndices;
    
    foreach (const FBXNode& child, node.children) {
        if (child.name == "Objects") {
            foreach (const FBXNode& object, child.children) {    
                if (object.name == "Geometry") {
                    if (object.properties.at(2) == "Mesh") {
                        FBXMesh mesh;
                    
                        QVector<int> polygonIndices;
                        QVector<glm::vec3> normals;
                        QVector<glm::vec2> texCoords;
                        QVector<int> texCoordIndices;
                        foreach (const FBXNode& data, object.children) {
                            if (data.name == "Vertices") {
                                mesh.vertices = createVec3Vector(data.properties.at(0).value<QVector<double> >());
                                
                            } else if (data.name == "PolygonVertexIndex") {
                                polygonIndices = data.properties.at(0).value<QVector<int> >();
                            
                            } else if (data.name == "LayerElementNormal") {
                                bool byVertex = false;
                                foreach (const FBXNode& subdata, data.children) {
                                    if (subdata.name == "Normals") {
                                        normals = createVec3Vector(subdata.properties.at(0).value<QVector<double> >());
                                    
                                    } else if (subdata.name == "MappingInformationType" &&
                                            subdata.properties.at(0) == "ByVertice") {
                                        byVertex = true;
                                    }
                                }
                                if (byVertex) {
                                    mesh.normals = normals;
                                }
                            } else if (data.name == "LayerElementUV" && data.properties.at(0).toInt() == 0) {
                                foreach (const FBXNode& subdata, data.children) {
                                    if (subdata.name == "UV") {
                                        texCoords = createVec2Vector(subdata.properties.at(0).value<QVector<double> >());
                                        
                                    } else if (subdata.name == "UVIndex") {
                                        texCoordIndices = subdata.properties.at(0).value<QVector<int> >();
                                    }
                                }
                            }
                        }
                        
                        // convert normals from per-index to per-vertex if necessary
                        if (mesh.normals.isEmpty()) {
                            mesh.normals.resize(mesh.vertices.size());
                            for (int i = 0, n = polygonIndices.size(); i < n; i++) {
                                int index = polygonIndices.at(i);
                                mesh.normals[index < 0 ? (-index - 1) : index] = normals.at(i);
                            } 
                        }
                        
                        // same with the tex coords
                        if (!texCoordIndices.isEmpty()) {
                            mesh.texCoords.resize(mesh.vertices.size());
                            for (int i = 0, n = polygonIndices.size(); i < n; i++) {
                                int index = polygonIndices.at(i);
                                int texCoordIndex = texCoordIndices.at(i);
                                if (texCoordIndex >= 0) {
                                    mesh.texCoords[index < 0 ? (-index - 1) : index] = texCoords.at(texCoordIndex); 
                                }
                            } 
                        }
                        
                        // convert the polygons to quads and triangles
                        for (const int* beginIndex = polygonIndices.constData(), *end = beginIndex + polygonIndices.size();
                                beginIndex != end; ) {
                            const int* endIndex = beginIndex;
                            while (*endIndex++ >= 0);
                            
                            if (endIndex - beginIndex == 4) {
                                mesh.quadIndices.append(*beginIndex++);
                                mesh.quadIndices.append(*beginIndex++);
                                mesh.quadIndices.append(*beginIndex++);
                                mesh.quadIndices.append(-*beginIndex++ - 1);
                                
                            } else {
                                for (const int* nextIndex = beginIndex + 1;; ) {
                                    mesh.triangleIndices.append(*beginIndex);
                                    mesh.triangleIndices.append(*nextIndex++);
                                    if (*nextIndex >= 0) {
                                        mesh.triangleIndices.append(*nextIndex);
                                    } else {
                                        mesh.triangleIndices.append(-*nextIndex - 1);
                                        break;
                                    }
                                }
                                beginIndex = endIndex;
                            }
                        }
                        meshes.insert(object.properties.at(0).value<qint64>(), mesh);
                        
                    } else { // object.properties.at(2) == "Shape"
                        ExtractedBlendshape extracted = { object.properties.at(0).value<qint64>() };
                        
                        foreach (const FBXNode& data, object.children) {
                            if (data.name == "Indexes") {
                                extracted.blendshape.indices = data.properties.at(0).value<QVector<int> >();
                                
                            } else if (data.name == "Vertices") {
                                extracted.blendshape.vertices = createVec3Vector(
                                    data.properties.at(0).value<QVector<double> >());
                                
                            } else if (data.name == "Normals") {
                                extracted.blendshape.normals = createVec3Vector(
                                    data.properties.at(0).value<QVector<double> >());
                            }
                        }
                        
                        blendshapes.append(extracted);
                    }
                } else if (object.name == "Model") {
                    QByteArray name = object.properties.at(1).toByteArray();
                    name = name.left(name.indexOf('\0'));
                    if (name == jointEyeLeftName || name == "EyeL" || name == "joint_Leye") {
                        jointEyeLeftID = object.properties.at(0).value<qint64>();
                        
                    } else if (name == jointEyeRightName || name == "EyeR" || name == "joint_Reye") {
                        jointEyeRightID = object.properties.at(0).value<qint64>();
                        
                    } else if (name == jointNeckName || name == "NeckRot" || name == "joint_neck") {
                        jointNeckID = object.properties.at(0).value<qint64>();
                    }
                    glm::vec3 translation;
                    glm::vec3 preRotation, rotation, postRotation;
                    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
                    glm::vec3 scalePivot, rotationPivot;
                    Transform transform = { true };
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "Properties70") {
                            foreach (const FBXNode& property, subobject.children) {
                                if (property.name == "P") {
                                    if (property.properties.at(0) == "Lcl Translation") {
                                        translation = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());

                                    } else if (property.properties.at(0) == "RotationPivot") {
                                        rotationPivot = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());
                                    
                                    } else if (property.properties.at(0) == "PreRotation") {
                                        preRotation = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());
                                            
                                    } else if (property.properties.at(0) == "Lcl Rotation") {
                                        rotation = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());
                                    
                                    } else if (property.properties.at(0) == "PostRotation") {
                                        postRotation = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());
                                        
                                    } else if (property.properties.at(0) == "ScalingPivot") {
                                        scalePivot = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());
                                            
                                    } else if (property.properties.at(0) == "Lcl Scaling") {
                                        scale = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());
                                            
                                    } else if (property.properties.at(0) == "InheritType") {
                                        transform.inheritScale = property.properties.at(4) != 2;
                                    }
                                }
                            }
                        }
                    }
                    // see FBX documentation, http://download.autodesk.com/us/fbx/20112/FBX_SDK_HELP/index.html
                    transform.withoutScale = glm::translate(translation) * glm::translate(rotationPivot) *
                        glm::mat4_cast(glm::quat(glm::radians(preRotation))) *
                        glm::mat4_cast(glm::quat(glm::radians(rotation))) *
                        glm::mat4_cast(glm::quat(glm::radians(postRotation))) * glm::translate(-rotationPivot);
                    transform.withScale = transform.withoutScale * glm::translate(scalePivot) * glm::scale(scale) *
                        glm::translate(-scalePivot);
                    localTransforms.insert(object.properties.at(0).value<qint64>(), transform);
                
                } else if (object.name == "Texture") {
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "RelativeFilename") {
                            // trim off any path information
                            QByteArray filename = subobject.properties.at(0).toByteArray();
                            filename = filename.mid(qMax(filename.lastIndexOf('\\'), filename.lastIndexOf('/')) + 1);
                            textureFilenames.insert(object.properties.at(0).value<qint64>(), filename);
                        }
                    }
                } else if (object.name == "Material") {
                    Material material = { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 96.0f };
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "Properties70") {        
                            foreach (const FBXNode& property, subobject.children) {
                                if (property.name == "P") {
                                    if (property.properties.at(0) == "DiffuseColor") {
                                        material.diffuse = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());
                                        
                                    } else if (property.properties.at(0) == "SpecularColor") {
                                        material.specular = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());
                                    
                                    } else if (property.properties.at(0) == "Shininess") {
                                        material.shininess = property.properties.at(4).value<double>();
                                    }
                                }
                            }
                        }
                    }
                    materials.insert(object.properties.at(0).value<qint64>(), material);
                    
                } else if (object.name == "Deformer") {
                    if (object.properties.at(2) == "Cluster") {
                        foreach (const FBXNode& subobject, object.children) {
                            if (subobject.name == "TransformLink") {
                                QVector<double> values = subobject.properties.at(0).value<QVector<double> >();
                                transformLinkMatrices.insert(object.properties.at(0).value<qint64>(), createMat4(values));
                            }
                        }
                    } else if (object.properties.at(2) == "BlendShapeChannel") {
                        QByteArray name = object.properties.at(1).toByteArray();
                        blendshapeChannelIndices.insert(object.properties.at(0).value<qint64>(),
                            blendshapeIndices.value(name.left(name.indexOf('\0'))));
                    }
                }
            }
        } else if (child.name == "Connections") {
            foreach (const FBXNode& connection, child.children) {    
                if (connection.name == "C") {
                    if (connection.properties.at(0) == "OP") {
                        if (connection.properties.at(3) == "DiffuseColor") { 
                            diffuseTextures.insert(connection.properties.at(2).value<qint64>(),
                                connection.properties.at(1).value<qint64>());
                                                    
                        } else if (connection.properties.at(3) == "Bump") {
                            bumpTextures.insert(connection.properties.at(2).value<qint64>(),
                                connection.properties.at(1).value<qint64>());
                        }
                    }
                    parentMap.insert(connection.properties.at(1).value<qint64>(), connection.properties.at(2).value<qint64>());
                    childMap.insert(connection.properties.at(2).value<qint64>(), connection.properties.at(1).value<qint64>());
                }
            }
        }
    }
    
    // assign the blendshapes to their corresponding meshes
    foreach (const ExtractedBlendshape& extracted, blendshapes) {
        qint64 blendshapeChannelID = parentMap.value(extracted.id);
        QPair<int, float> index = blendshapeChannelIndices.value(blendshapeChannelID);
        qint64 blendshapeID = parentMap.value(blendshapeChannelID);
        qint64 meshID = parentMap.value(blendshapeID);
        FBXMesh& mesh = meshes[meshID];
        mesh.blendshapes.resize(max(mesh.blendshapes.size(), index.first + 1));
        mesh.blendshapes[index.first] = extracted.blendshape;
    }
    
    // get offset transform from mapping
    float offsetScale = mapping.value("scale", 1.0f).toFloat();
    glm::mat4 offset = glm::translate(mapping.value("tx").toFloat(), mapping.value("ty").toFloat(),
        mapping.value("tz").toFloat()) * glm::mat4_cast(glm::quat(glm::radians(glm::vec3(mapping.value("rx").toFloat(),
            mapping.value("ry").toFloat(), mapping.value("rz").toFloat())))) *
        glm::scale(offsetScale, offsetScale, offsetScale);
    
    // as a temporary hack, put the mesh with the most blendshapes on top; assume it to be the face
    FBXGeometry geometry;
    int mostBlendshapes = 0;
    for (QHash<qint64, FBXMesh>::iterator it = meshes.begin(); it != meshes.end(); it++) {
        FBXMesh& mesh = it.value();
        
        // accumulate local transforms
        qint64 modelID = parentMap.value(it.key());
        glm::mat4 modelTransform = getGlobalTransform(parentMap, localTransforms, modelID);
        
        // look for textures, material properties
        foreach (qint64 childID, childMap.values(modelID)) {
            if (!materials.contains(childID)) {
                continue;
            }
            Material material = materials.value(childID);
            mesh.diffuseColor = material.diffuse;
            mesh.specularColor = material.specular;
            mesh.shininess = material.shininess;
            qint64 diffuseTextureID = diffuseTextures.value(childID);
            if (diffuseTextureID != 0) {
                mesh.diffuseFilename = textureFilenames.value(diffuseTextureID);
            }
            qint64 bumpTextureID = bumpTextures.value(childID);
            if (bumpTextureID != 0) {
                mesh.normalFilename = textureFilenames.value(bumpTextureID);
            }
        }
        
        // look for a limb pivot
        mesh.isEye = false;
        foreach (qint64 childID, childMap.values(it.key())) {
            foreach (qint64 clusterID, childMap.values(childID)) {
                if (!transformLinkMatrices.contains(clusterID)) {
                    continue;
                }
                qint64 jointID = childMap.value(clusterID);
                if (jointID == jointEyeLeftID || jointID == jointEyeRightID) {
                    mesh.isEye = true;
                }
                
                // see http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx for a discussion
                // of skinning information in FBX
                glm::mat4 jointTransform = offset * getGlobalTransform(parentMap, localTransforms, jointID);
                mesh.transform = jointTransform * glm::inverse(transformLinkMatrices.value(clusterID)) * modelTransform;
                
                // extract translation component for pivot
                glm::mat4 jointTransformScaled = offset * getGlobalTransform(parentMap, localTransforms, jointID, true);
                mesh.pivot = glm::vec3(jointTransformScaled[3][0], jointTransformScaled[3][1], jointTransformScaled[3][2]);
            }
        }
        
        if (mesh.blendshapes.size() > mostBlendshapes) {
            geometry.meshes.prepend(mesh);    
            mostBlendshapes = mesh.blendshapes.size();
            
        } else {
            geometry.meshes.append(mesh);
        }
    }
    
    // extract translation component for neck pivot
    glm::mat4 neckTransform = offset * getGlobalTransform(parentMap, localTransforms, jointNeckID, true);
    geometry.neckPivot = glm::vec3(neckTransform[3][0], neckTransform[3][1], neckTransform[3][2]);
    
    return geometry;
}

FBXGeometry readFBX(const QByteArray& model, const QByteArray& mapping) {
    QBuffer modelBuffer(const_cast<QByteArray*>(&model));
    modelBuffer.open(QIODevice::ReadOnly);
    
    QBuffer mappingBuffer(const_cast<QByteArray*>(&mapping));
    mappingBuffer.open(QIODevice::ReadOnly);
    
    return extractFBXGeometry(parseFBX(&modelBuffer), parseMapping(&mappingBuffer));
}

