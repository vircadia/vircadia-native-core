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

#include "FBXReader.h"

using namespace std;

FBXNode parseFBX(const QByteArray& data) {
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    return parseFBX(&buffer);
}

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

QVector<glm::vec3> createVec3Vector(const QVector<double>& doubleVector) {
    QVector<glm::vec3> values;
    for (const double* it = doubleVector.constData(), *end = it + doubleVector.size(); it != end; ) {
        values.append(glm::vec3(*it++, *it++, *it++));
    }
    return values;
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

QHash<QByteArray, int> createBlendshapeMap() {
    QHash<QByteArray, int> map;
    for (int i = 0;; i++) {
        QByteArray name = FACESHIFT_BLENDSHAPES[i];
        if (name != "") {
            map.insert(name, i);       
               
        } else {
            return map;
        }
    }
}

class ExtractedBlendshape {
public:
    qint64 id;
    int index;
    FBXBlendshape blendshape;
};

FBXGeometry extractFBXGeometry(const FBXNode& node) {
    QHash<qint64, FBXMesh> meshes;
    QVector<ExtractedBlendshape> blendshapes;
    QHash<qint64, qint64> parentMap;
    
    foreach (const FBXNode& child, node.children) {
        if (child.name == "Objects") {
            foreach (const FBXNode& object, child.children) {    
                if (object.name == "Geometry") {
                    if (object.properties.at(2) == "Mesh") {
                        FBXMesh mesh;
                    
                        QVector<glm::vec3> normals;
                        QVector<int> polygonIndices;
                        foreach (const FBXNode& data, object.children) {
                            if (data.name == "Vertices") {
                                mesh.vertices = createVec3Vector(data.properties.at(0).value<QVector<double> >());
                                
                            } else if (data.name == "PolygonVertexIndex") {
                                polygonIndices = data.properties.at(0).value<QVector<int> >();
                            
                            } else if (data.name == "LayerElementNormal") {
                                foreach (const FBXNode& subdata, data.children) {
                                    if (subdata.name == "Normals") {
                                        normals = createVec3Vector(subdata.properties.at(0).value<QVector<double> >());
                                    }
                                }    
                            }
                        }
                        
                        // the (base) normals correspond to the polygon indices, for some reason
                        mesh.normals.resize(mesh.vertices.size());
                        for (int i = 0, n = polygonIndices.size(); i < n; i++) {
                            int index = polygonIndices.at(i);
                            mesh.normals[index < 0 ? (-index - 1) : index] = normals[i];
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
                        
                        // the name is followed by a null and some type info
                        QByteArray name = object.properties.at(1).toByteArray();
                        static QHash<QByteArray, int> blendshapeMap = createBlendshapeMap();
                        extracted.index = blendshapeMap.value(name.left(name.indexOf('\0')));
                        
                        blendshapes.append(extracted);
                    }
                }
            }
        } else if (child.name == "Connections") {
            foreach (const FBXNode& connection, child.children) {    
                if (connection.name == "C") {
                    parentMap.insert(connection.properties.at(1).value<qint64>(), connection.properties.at(2).value<qint64>());
                }
            }
        }
    }
    
    // assign the blendshapes to their corresponding meshes
    foreach (const ExtractedBlendshape& extracted, blendshapes) {
        qint64 blendshapeChannelID = parentMap.value(extracted.id);
        qint64 blendshapeID = parentMap.value(blendshapeChannelID);
        qint64 meshID = parentMap.value(blendshapeID);
        FBXMesh& mesh = meshes[meshID];
        mesh.blendshapes.resize(max(mesh.blendshapes.size(), extracted.index + 1));
        mesh.blendshapes[extracted.index] = extracted.blendshape;
    }
    
    // as a temporary hack, put the mesh with the most blendshapes on top; assume it to be the face
    FBXGeometry geometry;
    int mostBlendshapes = 0;
    int mostBlendshapesIndex = 0;
    foreach (const FBXMesh& mesh, meshes) {
        if (mesh.blendshapes.size() > mostBlendshapes) {
            geometry.meshes.prepend(mesh);    
            mostBlendshapes = mesh.blendshapes.size();
            
        } else {
            geometry.meshes.append(mesh);
        }
    }
    
    return geometry;
}

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

