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
#include <QTextStream>
#include <QtDebug>
#include <QtEndian>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include "FBXReader.h"

using namespace std;

template<class T> QVariant readBinaryArray(QDataStream& in) {
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

QVariant parseBinaryFBXProperty(QDataStream& in) {
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
            return readBinaryArray<float>(in);
        }
        case 'd': {
            return readBinaryArray<double>(in);
        }
        case 'l': {
            return readBinaryArray<qint64>(in);
        }
        case 'i': {
            return readBinaryArray<qint32>(in);
        }
        case 'b': {
            return readBinaryArray<bool>(in);
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

FBXNode parseBinaryFBXNode(QDataStream& in) {
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
        node.properties.append(parseBinaryFBXProperty(in));    
    }
    
    while (endOffset > in.device()->pos()) {
        FBXNode child = parseBinaryFBXNode(in);
        if (child.name.isNull()) {
            return node;
            
        } else {
            node.children.append(child);
        }
    }
    
    return node;
}

class Tokenizer {
public:
    
    Tokenizer(QIODevice* device) : _device(device), _pushedBackToken(-1) { }
    
    enum SpecialToken { DATUM_TOKEN = 0x100 };
    
    int nextToken();
    const QByteArray& getDatum() const { return _datum; }
    
    void pushBackToken(int token) { _pushedBackToken = token; }
    
private:
    
    QIODevice* _device;
    QByteArray _datum;
    int _pushedBackToken;
};

int Tokenizer::nextToken() {
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
            case ';':
                _device->readLine(); // skip the comment
                break;
                
            case ':':
            case '{':
            case '}':
            case ',':
                return ch; // special punctuation
            
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
                    if (QChar(ch).isSpace() || ch == ';' || ch == ':' || ch == '{' || ch == '}' || ch == ',' || ch == '\"') {
                        _device->ungetChar(ch); // read until we encounter a special character, then replace it
                        break;
                    }
                    _datum.append(ch);
                }
                return DATUM_TOKEN;
        }
    }
    return -1;
}

FBXNode parseTextFBXNode(Tokenizer& tokenizer) {
    FBXNode node;
    
    if (tokenizer.nextToken() != Tokenizer::DATUM_TOKEN) {
        return node;
    }
    node.name = tokenizer.getDatum();
    
    if (tokenizer.nextToken() != ':') {
        return node;
    }
    
    int token;
    bool expectingDatum = true;
    while ((token = tokenizer.nextToken()) != -1) {
        if (token == '{') {
            for (FBXNode child = parseTextFBXNode(tokenizer); !child.name.isNull(); child = parseTextFBXNode(tokenizer)) {
                node.children.append(child);
            }
            return node;
        }
        if (token == ',') {
            expectingDatum = true;
            
        } else if (token == Tokenizer::DATUM_TOKEN && expectingDatum) {
            node.properties.append(tokenizer.getDatum());
            expectingDatum = false;
        
        } else {
            tokenizer.pushBackToken(token);
            return node;
        }
    }
    
    return node;
}

FBXNode parseFBX(QIODevice* device) {
    // verify the prolog
    const QByteArray BINARY_PROLOG = "Kaydara FBX Binary  ";
    if (device->peek(BINARY_PROLOG.size()) != BINARY_PROLOG) {
        // parse as a text file
        FBXNode top;
        Tokenizer tokenizer(device);
        while (device->bytesAvailable()) {    
            FBXNode next = parseTextFBXNode(tokenizer);
            if (next.name.isNull()) {
                return top;
                
            } else {
                top.children.append(next);
            }
        }
        return top;
    }
    QDataStream in(device);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setVersion(QDataStream::Qt_4_5); // for single/double precision switch
    
    // see http://code.blender.org/index.php/2013/08/fbx-binary-file-format-specification/ for an explanation
    // of the FBX binary format
    
    // skip the rest of the header
    const int HEADER_SIZE = 27;
    in.skipRawData(HEADER_SIZE);
    
    // parse the top-level node
    FBXNode top;
    while (device->bytesAvailable()) {
        FBXNode next = parseBinaryFBXNode(in);
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

QVector<int> getIntVector(const QVariantList& properties, int index) {
    QVector<int> vector = properties.at(index).value<QVector<int> >();
    if (!vector.isEmpty()) {
        return vector;
    }
    for (; index < properties.size(); index++) {
        vector.append(properties.at(index).toInt());
    }
    return vector;
}

QVector<double> getDoubleVector(const QVariantList& properties, int index) {
    QVector<double> vector = properties.at(index).value<QVector<double> >();
    if (!vector.isEmpty()) {
        return vector;
    }
    for (; index < properties.size(); index++) {
        vector.append(properties.at(index).toDouble());
    }
    return vector;
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

class FBXModel {
public:
    QByteArray name;
    
    glm::mat4 preRotation;
    glm::quat rotation;
    glm::mat4 postRotation;
    
    int parentIndex;
};

glm::mat4 getGlobalTransform(const QMultiHash<QString, QString>& parentMap,
        const QHash<QString, FBXModel>& models, QString nodeID) {
    glm::mat4 globalTransform;
    while (!nodeID.isNull()) {
        const FBXModel& model = models.value(nodeID);
        globalTransform = model.preRotation * glm::mat4_cast(model.rotation) * model.postRotation * globalTransform;
        
        QList<QString> parentIDs = parentMap.values(nodeID);
        nodeID = QString();
        foreach (const QString& parentID, parentIDs) {
            if (models.contains(parentID)) {
                nodeID = parentID;
                break;
            }
        }
    }
    
    return globalTransform;
}

class ExtractedBlendshape {
public:
    QString id;
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

class Cluster {
public:
    QVector<int> indices;
    QVector<double> weights;
    glm::mat4 transformLink;
};

void appendModelIDs(const QString& parentID, const QMultiHash<QString, QString>& childMap,
        QHash<QString, FBXModel>& models, QVector<QString>& modelIDs) {
    if (models.contains(parentID)) {
        modelIDs.append(parentID);
    }
    int parentIndex = modelIDs.size() - 1;
    foreach (const QString& childID, childMap.values(parentID)) {
        if (models.contains(childID)) {
            models[childID].parentIndex = parentIndex;
            appendModelIDs(childID, childMap, models, modelIDs);
        }
    }
}

FBXGeometry extractFBXGeometry(const FBXNode& node, const QVariantHash& mapping) {
    QHash<QString, FBXMesh> meshes;
    QVector<ExtractedBlendshape> blendshapes;
    QMultiHash<QString, QString> parentMap;
    QMultiHash<QString, QString> childMap;
    QHash<QString, FBXModel> models;
    QHash<QString, Cluster> clusters;
    QHash<QString, QByteArray> textureFilenames;
    QHash<QString, Material> materials;
    QHash<QString, QString> diffuseTextures;
    QHash<QString, QString> bumpTextures;
    
    QVariantHash joints = mapping.value("joint").toHash();
    QByteArray jointEyeLeftName = joints.value("jointEyeLeft", "jointEyeLeft").toByteArray();
    QByteArray jointEyeRightName = joints.value("jointEyeRight", "jointEyeRight").toByteArray();
    QByteArray jointNeckName = joints.value("jointNeck", "jointNeck").toByteArray();
    QString jointEyeLeftID;
    QString jointEyeRightID;
    QString jointNeckID;
    
    QVariantHash blendshapeMappings = mapping.value("bs").toHash();
    QHash<QByteArray, QPair<int, float> > blendshapeIndices;
    for (int i = 0;; i++) {
        QByteArray blendshapeName = FACESHIFT_BLENDSHAPES[i];
        if (blendshapeName.isEmpty()) {
            break;
        }
        QList<QVariant> mappings = blendshapeMappings.values(blendshapeName);
        if (mappings.isEmpty()) {
            blendshapeIndices.insert(blendshapeName, QPair<int, float>(i, 1.0f));
        } else {
            foreach (const QVariant& mapping, mappings) {
                QVariantList blendshapeMapping = mapping.toList();
                blendshapeIndices.insert(blendshapeMapping.at(0).toByteArray(),
                   QPair<int, float>(i, blendshapeMapping.at(1).toFloat()));
            }
        }
    }
    QHash<QString, QPair<int, float> > blendshapeChannelIndices;
    
    foreach (const FBXNode& child, node.children) {
        if (child.name == "Objects") {
            foreach (const FBXNode& object, child.children) {    
                if (object.name == "Geometry") {
                    if (object.properties.at(2) == "Mesh") {
                        FBXMesh mesh;
                    
                        QVector<int> polygonIndices;
                        QVector<glm::vec3> normals;
                        QVector<int> normalIndices;
                        QVector<glm::vec2> texCoords;
                        QVector<int> texCoordIndices;
                        QVector<int> materials;
                        foreach (const FBXNode& data, object.children) {
                            if (data.name == "Vertices") {
                                mesh.vertices = createVec3Vector(getDoubleVector(data.properties, 0));
                                
                            } else if (data.name == "PolygonVertexIndex") {
                                polygonIndices = getIntVector(data.properties, 0);
                            
                            } else if (data.name == "LayerElementNormal") {
                                bool byVertex = false;
                                foreach (const FBXNode& subdata, data.children) {
                                    if (subdata.name == "Normals") {
                                        normals = createVec3Vector(getDoubleVector(subdata.properties, 0));
                                    
                                    } else if (subdata.name == "NormalsIndex") {
                                        normalIndices = getIntVector(subdata.properties, 0);
                                        
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
                                        texCoords = createVec2Vector(getDoubleVector(subdata.properties, 0));
                                        
                                    } else if (subdata.name == "UVIndex") {
                                        texCoordIndices = getIntVector(subdata.properties, 0);
                                    }
                                }
                            } else if (data.name == "LayerElementMaterial") {
                                foreach (const FBXNode& subdata, data.children) {
                                    if (subdata.name == "Materials") {
                                        materials = getIntVector(subdata.properties, 0);   
                                    }
                                }
                            }
                        }
                        
                        // convert normals from per-index to per-vertex if necessary
                        if (mesh.normals.isEmpty()) {
                            mesh.normals.resize(mesh.vertices.size());
                            if (normalIndices.isEmpty()) {
                                for (int i = 0, n = polygonIndices.size(); i < n; i++) {
                                    int index = polygonIndices.at(i);
                                    mesh.normals[index < 0 ? (-index - 1) : index] = normals.at(i);
                                } 
                            } else {
                                for (int i = 0, n = polygonIndices.size(); i < n; i++) {
                                    int index = polygonIndices.at(i);
                                    int normalIndex = normalIndices.at(i);
                                    if (normalIndex >= 0) {
                                        mesh.normals[index < 0 ? (-index - 1) : index] = normals.at(normalIndex);
                                    }
                                } 
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
                        int polygonIndex = 0;
                        for (const int* beginIndex = polygonIndices.constData(), *end = beginIndex + polygonIndices.size();
                                beginIndex != end; polygonIndex++) {
                            const int* endIndex = beginIndex;
                            while (*endIndex++ >= 0);
                            
                            int materialIndex = (polygonIndex < materials.size()) ? materials.at(polygonIndex) : 0;
                            mesh.parts.resize(max(mesh.parts.size(), materialIndex + 1));
                            FBXMeshPart& part = mesh.parts[materialIndex];
                            
                            if (endIndex - beginIndex == 4) {
                                part.quadIndices.append(*beginIndex++);
                                part.quadIndices.append(*beginIndex++);
                                part.quadIndices.append(*beginIndex++);
                                part.quadIndices.append(-*beginIndex++ - 1);
                                
                            } else {
                                for (const int* nextIndex = beginIndex + 1;; ) {
                                    part.triangleIndices.append(*beginIndex);
                                    part.triangleIndices.append(*nextIndex++);
                                    if (*nextIndex >= 0) {
                                        part.triangleIndices.append(*nextIndex);
                                    } else {
                                        part.triangleIndices.append(-*nextIndex - 1);
                                        break;
                                    }
                                }
                                beginIndex = endIndex;
                            }
                        }
                        meshes.insert(object.properties.at(0).toString(), mesh);
                        
                    } else { // object.properties.at(2) == "Shape"
                        ExtractedBlendshape extracted = { object.properties.at(0).toString() };
                        
                        foreach (const FBXNode& data, object.children) {
                            if (data.name == "Indexes") {
                                extracted.blendshape.indices = getIntVector(data.properties, 0);
                                
                            } else if (data.name == "Vertices") {
                                extracted.blendshape.vertices = createVec3Vector(
                                    getDoubleVector(data.properties, 0));
                                
                            } else if (data.name == "Normals") {
                                extracted.blendshape.normals = createVec3Vector(
                                    getDoubleVector(data.properties, 0));
                            }
                        }
                        
                        blendshapes.append(extracted);
                    }
                } else if (object.name == "Model") {
                    QByteArray name = object.properties.at(1).toByteArray();
                    name = name.left(name.indexOf('\0'));
                    if (name == jointEyeLeftName || name == "EyeL" || name == "joint_Leye") {
                        jointEyeLeftID = object.properties.at(0).toString();
                        
                    } else if (name == jointEyeRightName || name == "EyeR" || name == "joint_Reye") {
                        jointEyeRightID = object.properties.at(0).toString();
                        
                    } else if (name == jointNeckName || name == "NeckRot" || name == "joint_neck") {
                        jointNeckID = object.properties.at(0).toString();
                    }
                    glm::vec3 translation;
                    glm::vec3 rotationOffset;
                    glm::vec3 preRotation, rotation, postRotation;
                    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
                    glm::vec3 scalePivot, rotationPivot;
                    FBXModel model = { name };
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "Properties60") {
                            foreach (const FBXNode& property, subobject.children) {
                                if (property.name == "Property") {
                                    if (property.properties.at(0) == "Lcl Translation") {
                                        translation = glm::vec3(property.properties.at(3).value<double>(),
                                            property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>());

                                    } else if (property.properties.at(0) == "RotationOffset") {
                                        rotationOffset = glm::vec3(property.properties.at(3).value<double>(),
                                            property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>());
                                            
                                    } else if (property.properties.at(0) == "RotationPivot") {
                                        rotationPivot = glm::vec3(property.properties.at(3).value<double>(),
                                            property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>());
                                    
                                    } else if (property.properties.at(0) == "PreRotation") {
                                        preRotation = glm::vec3(property.properties.at(3).value<double>(),
                                            property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>());
                                            
                                    } else if (property.properties.at(0) == "Lcl Rotation") {
                                        rotation = glm::vec3(property.properties.at(3).value<double>(),
                                            property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>());
                                    
                                    } else if (property.properties.at(0) == "PostRotation") {
                                        postRotation = glm::vec3(property.properties.at(3).value<double>(),
                                            property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>());
                                        
                                    } else if (property.properties.at(0) == "ScalingPivot") {
                                        scalePivot = glm::vec3(property.properties.at(3).value<double>(),
                                            property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>());
                                            
                                    } else if (property.properties.at(0) == "Lcl Scaling") {
                                        scale = glm::vec3(property.properties.at(3).value<double>(),
                                            property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>());       
                                    }
                                }
                            }
                        } else if (subobject.name == "Properties70") {
                            foreach (const FBXNode& property, subobject.children) {
                                if (property.name == "P") {
                                    if (property.properties.at(0) == "Lcl Translation") {
                                        translation = glm::vec3(property.properties.at(4).value<double>(),
                                            property.properties.at(5).value<double>(),
                                            property.properties.at(6).value<double>());

                                    } else if (property.properties.at(0) == "RotationOffset") {
                                        rotationOffset = glm::vec3(property.properties.at(4).value<double>(),
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
                                    }
                                }
                            }
                        }
                    }
                    // see FBX documentation, http://download.autodesk.com/us/fbx/20112/FBX_SDK_HELP/index.html
                    model.preRotation = glm::translate(translation) * glm::translate(rotationOffset) *
                        glm::translate(rotationPivot) * glm::mat4_cast(glm::quat(glm::radians(preRotation)));                   
                    model.rotation = glm::quat(glm::radians(rotation));
                    model.postRotation = glm::mat4_cast(glm::quat(glm::radians(postRotation))) *
                        glm::translate(-rotationPivot) * glm::translate(scalePivot) *
                        glm::scale(scale) * glm::translate(-scalePivot);
                    models.insert(object.properties.at(0).toString(), model);

                } else if (object.name == "Texture") {
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "RelativeFilename") {
                            // trim off any path information
                            QByteArray filename = subobject.properties.at(0).toByteArray();
                            filename = filename.mid(qMax(filename.lastIndexOf('\\'), filename.lastIndexOf('/')) + 1);
                            textureFilenames.insert(object.properties.at(0).toString(), filename);
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
                    materials.insert(object.properties.at(0).toString(), material);
                    
                } else if (object.name == "Deformer") {
                    if (object.properties.last() == "Cluster") {
                        Cluster cluster;
                        foreach (const FBXNode& subobject, object.children) {
                            if (subobject.name == "Indexes") {
                                cluster.indices = getIntVector(subobject.properties, 0);
                                
                            } else if (subobject.name == "Weights") {
                                cluster.weights = getDoubleVector(subobject.properties, 0);
                            
                            } else if (subobject.name == "TransformLink") {
                                QVector<double> values = getDoubleVector(subobject.properties, 0);
                                cluster.transformLink = createMat4(values);
                            }
                        }
                        clusters.insert(object.properties.at(0).toString(), cluster);
                        
                    } else if (object.properties.last() == "BlendShapeChannel") {
                        QByteArray name = object.properties.at(1).toByteArray();
                        name = name.left(name.indexOf('\0'));
                        if (!blendshapeIndices.contains(name)) {
                            // try everything after the dot
                            name = name.mid(name.lastIndexOf('.') + 1);
                        }
                        blendshapeChannelIndices.insert(object.properties.at(0).toString(),
                            blendshapeIndices.value(name));
                    }
                }
            }
        } else if (child.name == "Connections") {
            foreach (const FBXNode& connection, child.children) {    
                if (connection.name == "C" || connection.name == "Connect") {
                    if (connection.properties.at(0) == "OP") {
                        if (connection.properties.at(3) == "DiffuseColor") { 
                            diffuseTextures.insert(connection.properties.at(2).toString(),
                                connection.properties.at(1).toString());
                                                    
                        } else if (connection.properties.at(3) == "Bump") {
                            bumpTextures.insert(connection.properties.at(2).toString(),
                                connection.properties.at(1).toString());
                        }
                    }
                    parentMap.insert(connection.properties.at(1).toString(), connection.properties.at(2).toString());
                    childMap.insert(connection.properties.at(2).toString(), connection.properties.at(1).toString());
                }
            }
        }
    }
    
    // assign the blendshapes to their corresponding meshes
    foreach (const ExtractedBlendshape& extracted, blendshapes) {
        QString blendshapeChannelID = parentMap.value(extracted.id);
        QPair<int, float> index = blendshapeChannelIndices.value(blendshapeChannelID);
        QString blendshapeID = parentMap.value(blendshapeChannelID);
        QString meshID = parentMap.value(blendshapeID);
        FBXMesh& mesh = meshes[meshID];
        mesh.blendshapes.resize(max(mesh.blendshapes.size(), index.first + 1));
        mesh.blendshapes[index.first] = extracted.blendshape;
        
        // apply scale if non-unity
        if (index.second != 1.0f) {
            FBXBlendshape& blendshape = mesh.blendshapes[index.first];
            for (int i = 0; i < blendshape.vertices.size(); i++) {
                blendshape.vertices[i] *= index.second;
                blendshape.normals[i] *= index.second;
            }
        }
    }
    
    // get offset transform from mapping
    FBXGeometry geometry;
    float offsetScale = mapping.value("scale", 1.0f).toFloat();
    geometry.offset = glm::translate(mapping.value("tx").toFloat(), mapping.value("ty").toFloat(),
        mapping.value("tz").toFloat()) * glm::mat4_cast(glm::quat(glm::radians(glm::vec3(mapping.value("rx").toFloat(),
            mapping.value("ry").toFloat(), mapping.value("rz").toFloat())))) *
        glm::scale(offsetScale, offsetScale, offsetScale);
    
    // get the list of models in depth-first traversal order
    QVector<QString> modelIDs;
    if (!models.isEmpty()) {
        QString top = models.constBegin().key();
        forever {
            foreach (const QString& name, parentMap.values(top)) {
                if (models.contains(name)) {
                    top = name;
                    goto outerContinue;
                }
            }
            top = parentMap.value(top);
            break;
            
            outerContinue: ;
        }
        appendModelIDs(top, childMap, models, modelIDs);
    }
    
    // convert the models to joints
    foreach (const QString& modelID, modelIDs) {
        const FBXModel& model = models[modelID];
        FBXJoint joint;
        joint.parentIndex = model.parentIndex;
        joint.preRotation = model.preRotation;
        joint.rotation = model.rotation;
        joint.postRotation = model.postRotation;
        if (joint.parentIndex == -1) {
            joint.transform = geometry.offset * model.preRotation * glm::mat4_cast(model.rotation) * model.postRotation;
            
        } else {
            joint.transform = geometry.joints.at(joint.parentIndex).transform *
                model.preRotation * glm::mat4_cast(model.rotation) * model.postRotation;
        }
        geometry.joints.append(joint);
        geometry.jointIndices.insert(model.name, geometry.joints.size() - 1);  
    }
    
    // find our special joints
    geometry.leftEyeJointIndex = modelIDs.indexOf(jointEyeLeftID);
    geometry.rightEyeJointIndex = modelIDs.indexOf(jointEyeRightID);
    geometry.neckJointIndex = modelIDs.indexOf(jointNeckID);
    
    // extract the translation component of the neck transform
    if (geometry.neckJointIndex != -1) {
        const glm::mat4& transform = geometry.joints.at(geometry.neckJointIndex).transform;
        geometry.neckPivot = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
    }
    
    QVariantHash springs = mapping.value("spring").toHash();
    QVariant defaultSpring = springs.value("default");
    for (QHash<QString, FBXMesh>::iterator it = meshes.begin(); it != meshes.end(); it++) {
        FBXMesh& mesh = it.value();
        
        // accumulate local transforms
        QString modelID = parentMap.value(it.key());
        mesh.springiness = springs.value(models.value(modelID).name, defaultSpring).toFloat();
        glm::mat4 modelTransform = getGlobalTransform(parentMap, models, modelID);
        
        // look for textures, material properties
        int partIndex = 0;
        foreach (const QString& childID, childMap.values(modelID)) {
            if (!materials.contains(childID) || partIndex >= mesh.parts.size()) {
                continue;
            }
            Material material = materials.value(childID);
            FBXMeshPart& part = mesh.parts[mesh.parts.size() - ++partIndex];
            part.diffuseColor = material.diffuse;
            part.specularColor = material.specular;
            part.shininess = material.shininess;
            QString diffuseTextureID = diffuseTextures.value(childID);
            if (!diffuseTextureID.isNull()) {
                part.diffuseFilename = textureFilenames.value(diffuseTextureID);
            }
            QString bumpTextureID = bumpTextures.value(childID);
            if (!bumpTextureID.isNull()) {
                part.normalFilename = textureFilenames.value(bumpTextureID);
            }
        }
        
        // find the clusters with which the mesh is associated
        mesh.isEye = false;
        QVector<QString> clusterIDs;
        foreach (const QString& childID, childMap.values(it.key())) {
            foreach (const QString& clusterID, childMap.values(childID)) {
                if (!clusters.contains(clusterID)) {
                    continue;
                }
                FBXCluster fbxCluster;
                const Cluster& cluster = clusters[clusterID];
                clusterIDs.append(clusterID);
                
                QString jointID = childMap.value(clusterID);
                if (jointID == jointEyeLeftID || jointID == jointEyeRightID) {
                    mesh.isEye = true;
                }
                // see http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx for a discussion
                // of skinning information in FBX
                fbxCluster.jointIndex = modelIDs.indexOf(jointID);
                fbxCluster.inverseBindMatrix = glm::inverse(cluster.transformLink) * modelTransform;
                mesh.clusters.append(fbxCluster);
            }
        }
        
        // if we don't have a skinned joint, parent to the model itself
        if (mesh.clusters.isEmpty()) {
            FBXCluster cluster;
            cluster.jointIndex = modelIDs.indexOf(modelID);
            mesh.clusters.append(cluster);
        }
        
        // whether we're skinned depends on how many clusters are attached
        if (clusterIDs.size() > 1) {
            mesh.clusterIndices.resize(mesh.vertices.size());
            mesh.clusterWeights.resize(mesh.vertices.size());
            for (int i = 0; i < clusterIDs.size(); i++) {
                QString clusterID = clusterIDs.at(i);
                const Cluster& cluster = clusters[clusterID];
                
                for (int j = 0; j < cluster.indices.size(); j++) {
                    int index = cluster.indices.at(j);
                    glm::vec4& weights = mesh.clusterWeights[index];
                    
                    // look for an unused slot in the weights vector
                    for (int k = 0; k < 4; k++) {
                        if (weights[k] == 0.0f) {
                            mesh.clusterIndices[index][k] = i;
                            weights[k] = cluster.weights.at(j);
                            break;
                        }
                    }
                }
            }
        }
        
        // extract spring edges, connections if springy
        if (mesh.springiness > 0.0f) {
            QSet<QPair<int, int> > edges;
            
            mesh.vertexConnections.resize(mesh.vertices.size());
            foreach (const FBXMeshPart& part, mesh.parts) {
                for (int i = 0; i < part.quadIndices.size(); i += 4) {
                    int index0 = part.quadIndices.at(i);
                    int index1 = part.quadIndices.at(i + 1);
                    int index2 = part.quadIndices.at(i + 2);
                    int index3 = part.quadIndices.at(i + 3);
                    
                    edges.insert(QPair<int, int>(qMin(index0, index1), qMax(index0, index1)));
                    edges.insert(QPair<int, int>(qMin(index1, index2), qMax(index1, index2)));
                    edges.insert(QPair<int, int>(qMin(index2, index3), qMax(index2, index3)));
                    edges.insert(QPair<int, int>(qMin(index3, index0), qMax(index3, index0)));
               
                    mesh.vertexConnections[index0].append(QPair<int, int>(index3, index1));
                    mesh.vertexConnections[index1].append(QPair<int, int>(index0, index2));
                    mesh.vertexConnections[index2].append(QPair<int, int>(index1, index3));
                    mesh.vertexConnections[index3].append(QPair<int, int>(index2, index0));
                }
                for (int i = 0; i < part.triangleIndices.size(); i += 3) {
                    int index0 = part.triangleIndices.at(i);
                    int index1 = part.triangleIndices.at(i + 1);
                    int index2 = part.triangleIndices.at(i + 2);
                    
                    edges.insert(QPair<int, int>(qMin(index0, index1), qMax(index0, index1)));
                    edges.insert(QPair<int, int>(qMin(index1, index2), qMax(index1, index2)));
                    edges.insert(QPair<int, int>(qMin(index2, index0), qMax(index2, index0)));
                    
                    mesh.vertexConnections[index0].append(QPair<int, int>(index2, index1));
                    mesh.vertexConnections[index1].append(QPair<int, int>(index0, index2));
                    mesh.vertexConnections[index2].append(QPair<int, int>(index1, index0));
                }
            }
            
            for (QSet<QPair<int, int> >::const_iterator edge = edges.constBegin(); edge != edges.constEnd(); edge++) {
                mesh.springEdges.append(*edge);
            }
        }
        
        geometry.meshes.append(mesh);
    }
    
    return geometry;
}

FBXGeometry readFBX(const QByteArray& model, const QByteArray& mapping) {
    QBuffer modelBuffer(const_cast<QByteArray*>(&model));
    modelBuffer.open(QIODevice::ReadOnly);
    
    QBuffer mappingBuffer(const_cast<QByteArray*>(&mapping));
    mappingBuffer.open(QIODevice::ReadOnly);
    
    return extractFBXGeometry(parseFBX(&modelBuffer), parseMapping(&mappingBuffer));
}

