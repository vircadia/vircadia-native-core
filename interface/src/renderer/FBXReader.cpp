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
#include <QStringList>
#include <QTextStream>
#include <QtDebug>
#include <QtEndian>

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

#include <OctalCode.h>

#include <VoxelTree.h>

#include "FBXReader.h"
#include "Util.h"

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
            properties.insertMulti(name, sections.at(1).trimmed());
        
        } else if (sections.size() == 3) {
            QVariantHash heading = properties.value(name).toHash();
            heading.insertMulti(sections.at(1).trimmed(), sections.at(2).trimmed());
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

glm::vec3 getVec3(const QVariantList& properties, int index) {
    return glm::vec3(properties.at(index).value<double>(), properties.at(index + 1).value<double>(),
        properties.at(index + 2).value<double>());
}

glm::vec3 parseVec3(const QString& string) {
    QStringList elements = string.split(',');
    if (elements.isEmpty()) {
        return glm::vec3();
    }
    glm::vec3 value;
    for (int i = 0; i < 3; i++) {
        // duplicate last value if there aren't three elements
        value[i] = elements.at(min(i, elements.size() - 1)).trimmed().toFloat();
    }
    return value;
}

QString processID(const QString& id) {
    // Blender (at least) prepends a type to the ID, so strip it out
    int index = id.indexOf("::");
    return (index == -1) ? id : id.mid(index + 2);
}

QString getID(const QVariantList& properties, int index = 0) {
    return processID(properties.at(index).toString());
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
    QString name;
    
    int parentIndex;
    
    glm::mat4 preTransform;
    glm::quat preRotation;
    glm::quat rotation;
    glm::quat postRotation;
    glm::mat4 postTransform;
};

glm::mat4 getGlobalTransform(const QMultiHash<QString, QString>& parentMap,
        const QHash<QString, FBXModel>& models, QString nodeID) {
    glm::mat4 globalTransform;
    while (!nodeID.isNull()) {
        const FBXModel& model = models.value(nodeID);
        globalTransform = model.preTransform * glm::mat4_cast(model.preRotation * model.rotation * model.postRotation) *
            model.postTransform * globalTransform;
        
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
            FBXModel& model = models[childID];
            if (model.parentIndex == -1) {
                model.parentIndex = parentIndex;
                appendModelIDs(childID, childMap, models, modelIDs);
            }
        }
    }
}

class Vertex {
public:
    int originalIndex;
    glm::vec2 texCoord;
};

uint qHash(const Vertex& vertex, uint seed = 0) {
    return qHash(vertex.originalIndex, seed);
}

bool operator==(const Vertex& v1, const Vertex& v2) {
    return v1.originalIndex == v2.originalIndex && v1.texCoord == v2.texCoord;
}

class ExtractedMesh {
public:
    FBXMesh mesh;
    QMultiHash<int, int> newIndices;
};

class MeshData {
public:
    ExtractedMesh extracted;
    QVector<glm::vec3> vertices;
    QVector<int> polygonIndices;
    bool normalsByVertex;
    QVector<glm::vec3> normals;
    QVector<int> normalIndices;
    QVector<glm::vec2> texCoords;
    QVector<int> texCoordIndices;
    
    QHash<Vertex, int> indices;
};

void appendIndex(MeshData& data, QVector<int>& indices, int index) {
    int vertexIndex = data.polygonIndices.at(index);
    if (vertexIndex < 0) {
        vertexIndex = -vertexIndex - 1;
    }
    
    Vertex vertex;
    vertex.originalIndex = vertexIndex;

    glm::vec3 normal;
    if (data.normalIndices.isEmpty()) {
        normal = data.normals.at(data.normalsByVertex ? vertexIndex : index);
        
    } else {
        int normalIndex = data.normalIndices.at(data.normalsByVertex ? vertexIndex : index);
        if (normalIndex >= 0) {
            normal = data.normals.at(normalIndex);
        }
    }

    if (data.texCoordIndices.isEmpty()) {
        if (index < data.texCoords.size()) {
            vertex.texCoord = data.texCoords.at(index);    
        }
    } else {
        int texCoordIndex = data.texCoordIndices.at(index);
        if (texCoordIndex >= 0) {
            vertex.texCoord = data.texCoords.at(texCoordIndex); 
        }
    }
    
    QHash<Vertex, int>::const_iterator it = data.indices.find(vertex);
    if (it == data.indices.constEnd()) {
        int newIndex = data.extracted.mesh.vertices.size();
        indices.append(newIndex);
        data.indices.insert(vertex, newIndex);
        data.extracted.newIndices.insert(vertexIndex, newIndex);
        data.extracted.mesh.vertices.append(data.vertices.at(vertexIndex));
        data.extracted.mesh.normals.append(normal);
        data.extracted.mesh.texCoords.append(vertex.texCoord);
        
    } else {
        indices.append(*it);
        data.extracted.mesh.normals[*it] += normal;
    }
}

ExtractedMesh extractMesh(const FBXNode& object) {                
    MeshData data;
    QVector<int> materials;
    foreach (const FBXNode& child, object.children) {
        if (child.name == "Vertices") {
            data.vertices = createVec3Vector(getDoubleVector(child.properties, 0));
            
        } else if (child.name == "PolygonVertexIndex") {
            data.polygonIndices = getIntVector(child.properties, 0);
        
        } else if (child.name == "LayerElementNormal") {
            data.normalsByVertex = false;
            foreach (const FBXNode& subdata, child.children) {
                if (subdata.name == "Normals") {
                    data.normals = createVec3Vector(getDoubleVector(subdata.properties, 0));
                
                } else if (subdata.name == "NormalsIndex") {
                    data.normalIndices = getIntVector(subdata.properties, 0);
                    
                } else if (subdata.name == "MappingInformationType" &&
                        subdata.properties.at(0) == "ByVertice") {
                    data.normalsByVertex = true;
                }
            }
        } else if (child.name == "LayerElementUV" && child.properties.at(0).toInt() == 0) {
            foreach (const FBXNode& subdata, child.children) {
                if (subdata.name == "UV") {
                    data.texCoords = createVec2Vector(getDoubleVector(subdata.properties, 0));
                    
                } else if (subdata.name == "UVIndex") {
                    data.texCoordIndices = getIntVector(subdata.properties, 0);
                }
            }
        } else if (child.name == "LayerElementMaterial") {
            foreach (const FBXNode& subdata, child.children) {
                if (subdata.name == "Materials") {
                    materials = getIntVector(subdata.properties, 0);   
                }
            }
        }
    }
    
    // convert the polygons to quads and triangles
    int polygonIndex = 0;
    for (int beginIndex = 0; beginIndex < data.polygonIndices.size(); polygonIndex++) {
        int endIndex = beginIndex;
        while (data.polygonIndices.at(endIndex++) >= 0);
        
        int materialIndex = (polygonIndex < materials.size()) ? materials.at(polygonIndex) : 0;
        data.extracted.mesh.parts.resize(max(data.extracted.mesh.parts.size(), materialIndex + 1));
        FBXMeshPart& part = data.extracted.mesh.parts[materialIndex];
        
        if (endIndex - beginIndex == 4) {
            appendIndex(data, part.quadIndices, beginIndex++);
            appendIndex(data, part.quadIndices, beginIndex++);
            appendIndex(data, part.quadIndices, beginIndex++);
            appendIndex(data, part.quadIndices, beginIndex++);
            
        } else {
            for (int nextIndex = beginIndex + 1;; ) {
                appendIndex(data, part.triangleIndices, beginIndex);
                appendIndex(data, part.triangleIndices, nextIndex++);
                appendIndex(data, part.triangleIndices, nextIndex);
                if (data.polygonIndices.at(nextIndex) < 0) {
                    break;
                }
            }
            beginIndex = endIndex;
        }
    }
    
    return data.extracted;
}

void setTangents(FBXMesh& mesh, int firstIndex, int secondIndex) {
    glm::vec3 normal = glm::normalize(mesh.normals.at(firstIndex));
    glm::vec3 bitangent = glm::cross(normal, mesh.vertices.at(secondIndex) - mesh.vertices.at(firstIndex));
    if (glm::length(bitangent) < EPSILON) {
        return;
    }
    glm::vec2 texCoordDelta = mesh.texCoords.at(secondIndex) - mesh.texCoords.at(firstIndex);
    mesh.tangents[firstIndex] += glm::cross(glm::angleAxis(
        -glm::degrees(atan2f(-texCoordDelta.t, texCoordDelta.s)), normal) * glm::normalize(bitangent), normal);
}

FBXGeometry extractFBXGeometry(const FBXNode& node, const QVariantHash& mapping) {
    QHash<QString, ExtractedMesh> meshes;
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
    QString jointEyeLeftName = processID(joints.value("jointEyeLeft", "jointEyeLeft").toString());
    QString jointEyeRightName = processID(joints.value("jointEyeRight", "jointEyeRight").toString());
    QString jointNeckName = processID(joints.value("jointNeck", "jointNeck").toString());
    QString jointRootName = processID(joints.value("jointRoot", "jointRoot").toString());
    QString jointLeanName = processID(joints.value("jointLean", "jointLean").toString());
    QString jointHeadName = processID(joints.value("jointHead", "jointHead").toString());
    QString jointLeftHandName = processID(joints.value("jointLeftHand", "jointLeftHand").toString());
    QString jointRightHandName = processID(joints.value("jointRightHand", "jointRightHand").toString());
    QString jointEyeLeftID;
    QString jointEyeRightID;
    QString jointNeckID;
    QString jointRootID;
    QString jointLeanID;
    QString jointHeadID;
    QString jointLeftHandID;
    QString jointRightHandID;
    
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
                        meshes.insert(getID(object.properties), extractMesh(object));
                        
                    } else { // object.properties.at(2) == "Shape"
                        ExtractedBlendshape extracted = { getID(object.properties) };
                        
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
                    QString name;
                    if (object.properties.size() == 3) {
                        name = object.properties.at(1).toString();
                        name = name.left(name.indexOf(QChar('\0')));
                        
                    } else {
                        name = getID(object.properties);
                    }
                    if (name == jointEyeLeftName || name == "EyeL" || name == "joint_Leye") {
                        jointEyeLeftID = getID(object.properties);
                        
                    } else if (name == jointEyeRightName || name == "EyeR" || name == "joint_Reye") {
                        jointEyeRightID = getID(object.properties);
                        
                    } else if (name == jointNeckName || name == "NeckRot" || name == "joint_neck") {
                        jointNeckID = getID(object.properties);
                        
                    } else if (name == jointRootName) {
                        jointRootID = getID(object.properties);
                        
                    } else if (name == jointLeanName) {
                        jointLeanID = getID(object.properties);
                        
                    } else if (name == jointHeadName) {
                        jointHeadID = getID(object.properties);
                        
                    } else if (name == jointLeftHandName) {
                        jointLeftHandID = getID(object.properties);
                        
                    } else if (name == jointRightHandName) {
                        jointRightHandID = getID(object.properties);
                    }
                    glm::vec3 translation;
                    glm::vec3 rotationOffset;
                    glm::vec3 preRotation, rotation, postRotation;
                    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
                    glm::vec3 scalePivot, rotationPivot;
                    FBXModel model = { name, -1 };
                    foreach (const FBXNode& subobject, object.children) {
                        bool properties = false;
                        QByteArray propertyName;
                        int index;
                        if (subobject.name == "Properties60") {
                            properties = true;
                            propertyName = "Property";
                            index = 3;
                            
                        } else if (subobject.name == "Properties70") {
                            properties = true;
                            propertyName = "P";
                            index = 4;
                        }
                        if (properties) {
                            foreach (const FBXNode& property, subobject.children) {
                                if (property.name == propertyName) {
                                    if (property.properties.at(0) == "Lcl Translation") {
                                        translation = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "RotationOffset") {
                                        rotationOffset = getVec3(property.properties, index);
                                            
                                    } else if (property.properties.at(0) == "RotationPivot") {
                                        rotationPivot = getVec3(property.properties, index);
                                    
                                    } else if (property.properties.at(0) == "PreRotation") {
                                        preRotation = getVec3(property.properties, index);
                                            
                                    } else if (property.properties.at(0) == "Lcl Rotation") {
                                        rotation = getVec3(property.properties, index);
                                    
                                    } else if (property.properties.at(0) == "PostRotation") {
                                        postRotation = getVec3(property.properties, index);
                                        
                                    } else if (property.properties.at(0) == "ScalingPivot") {
                                        scalePivot = getVec3(property.properties, index);
                                            
                                    } else if (property.properties.at(0) == "Lcl Scaling") {
                                        scale = getVec3(property.properties, index);     
                                    }
                                }
                            }
                        } else if (subobject.name == "Vertices") {
                            // it's a mesh as well as a model
                            meshes.insert(getID(object.properties), extractMesh(object));
                        }
                    }
                    // see FBX documentation, http://download.autodesk.com/us/fbx/20112/FBX_SDK_HELP/index.html
                    model.preTransform = glm::translate(translation) * glm::translate(rotationOffset) *
                        glm::translate(rotationPivot);      
                    model.preRotation = glm::quat(glm::radians(preRotation));            
                    model.rotation = glm::quat(glm::radians(rotation));
                    model.postRotation = glm::quat(glm::radians(postRotation));
                    model.postTransform = glm::translate(-rotationPivot) * glm::translate(scalePivot) *
                        glm::scale(scale) * glm::translate(-scalePivot);
                    models.insert(getID(object.properties), model);

                } else if (object.name == "Texture") {
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "RelativeFilename") {
                            // trim off any path information
                            QByteArray filename = subobject.properties.at(0).toByteArray();
                            filename = filename.mid(qMax(filename.lastIndexOf('\\'), filename.lastIndexOf('/')) + 1);
                            textureFilenames.insert(getID(object.properties), filename);
                        }
                    }
                } else if (object.name == "Material") {
                    Material material = { glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(1.0f, 1.0f, 1.0f), 96.0f };
                    foreach (const FBXNode& subobject, object.children) {
                        bool properties = false;
                        QByteArray propertyName;
                        int index;
                        if (subobject.name == "Properties60") {
                            properties = true;
                            propertyName = "Property";
                            index = 3;
                            
                        } else if (subobject.name == "Properties70") {
                            properties = true;
                            propertyName = "P";
                            index = 4;
                        }
                        if (properties) {        
                            foreach (const FBXNode& property, subobject.children) {
                                if (property.name == propertyName) {
                                    if (property.properties.at(0) == "DiffuseColor") {
                                        material.diffuse = getVec3(property.properties, index);
                                        
                                    } else if (property.properties.at(0) == "SpecularColor") {
                                        material.specular = getVec3(property.properties, index);
                                    
                                    } else if (property.properties.at(0) == "Shininess") {
                                        material.shininess = property.properties.at(index).value<double>();
                                    }
                                }
                            }
                        }
                    }
                    materials.insert(getID(object.properties), material);
                    
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
                        clusters.insert(getID(object.properties), cluster);
                        
                    } else if (object.properties.last() == "BlendShapeChannel") {
                        QByteArray name = object.properties.at(1).toByteArray();
                        name = name.left(name.indexOf('\0'));
                        if (!blendshapeIndices.contains(name)) {
                            // try everything after the dot
                            name = name.mid(name.lastIndexOf('.') + 1);
                        }
                        blendshapeChannelIndices.insert(getID(object.properties), blendshapeIndices.value(name));
                    }
                }
            }
        } else if (child.name == "Connections") {
            foreach (const FBXNode& connection, child.children) {    
                if (connection.name == "C" || connection.name == "Connect") {
                    if (connection.properties.at(0) == "OP") {
                        QByteArray type = connection.properties.at(3).toByteArray().toLower();
                        if (type.contains("diffuse")) { 
                            diffuseTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                                                    
                        } else if (type.contains("bump")) {
                            bumpTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        }
                    }
                    parentMap.insert(getID(connection.properties, 1), getID(connection.properties, 2));
                    childMap.insert(getID(connection.properties, 2), getID(connection.properties, 1));
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
        ExtractedMesh& extractedMesh = meshes[meshID];
        extractedMesh.mesh.blendshapes.resize(max(extractedMesh.mesh.blendshapes.size(), index.first + 1));
        FBXBlendshape& blendshape = extractedMesh.mesh.blendshapes[index.first];
        for (int i = 0; i < extracted.blendshape.indices.size(); i++) {
            int oldIndex = extracted.blendshape.indices.at(i);
            for (QMultiHash<int, int>::const_iterator it = extractedMesh.newIndices.constFind(oldIndex);
                    it != extractedMesh.newIndices.constEnd() && it.key() == oldIndex; it++) {
                blendshape.indices.append(it.value());
                blendshape.vertices.append(extracted.blendshape.vertices.at(i) * index.second);
                blendshape.normals.append(extracted.blendshape.normals.at(i) * index.second);
            } 
        }
    }
    
    // get offset transform from mapping
    FBXGeometry geometry;
    float offsetScale = mapping.value("scale", 1.0f).toFloat();
    glm::quat offsetRotation = glm::quat(glm::radians(glm::vec3(mapping.value("rx").toFloat(),
            mapping.value("ry").toFloat(), mapping.value("rz").toFloat())));
    geometry.offset = glm::translate(mapping.value("tx").toFloat(), mapping.value("ty").toFloat(),
        mapping.value("tz").toFloat()) * glm::mat4_cast(offsetRotation) * glm::scale(offsetScale, offsetScale, offsetScale);
    
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
    QVariantList freeJoints = mapping.values("freeJoint");
    foreach (const QString& modelID, modelIDs) {
        const FBXModel& model = models[modelID];
        FBXJoint joint;
        joint.isFree = freeJoints.contains(model.name);
        joint.parentIndex = model.parentIndex;
        
        // get the indices of all ancestors starting with the first free one (if any)
        joint.freeLineage.append(geometry.joints.size());
        int lastFreeIndex = joint.isFree ? 0 : -1;
        for (int index = joint.parentIndex; index != -1; index = geometry.joints.at(index).parentIndex) {
            if (geometry.joints.at(index).isFree) {
                lastFreeIndex = joint.freeLineage.size();
            }
            joint.freeLineage.append(index);
        }
        joint.freeLineage.remove(lastFreeIndex + 1, joint.freeLineage.size() - lastFreeIndex - 1);
        
        joint.preTransform = model.preTransform;
        joint.preRotation = model.preRotation;
        joint.rotation = model.rotation;
        joint.postRotation = model.postRotation;
        joint.postTransform = model.postTransform;
        glm::quat combinedRotation = model.preRotation * model.rotation * model.postRotation;
        if (joint.parentIndex == -1) {    
            joint.transform = geometry.offset * model.preTransform * glm::mat4_cast(combinedRotation) * model.postTransform;
            joint.inverseBindRotation = glm::inverse(combinedRotation);
            
        } else {
            const FBXJoint& parentJoint = geometry.joints.at(joint.parentIndex);
            joint.transform = parentJoint.transform *
                model.preTransform * glm::mat4_cast(combinedRotation) * model.postTransform;
            joint.inverseBindRotation = glm::inverse(combinedRotation) * parentJoint.inverseBindRotation;
        }
        geometry.joints.append(joint);
        geometry.jointIndices.insert(model.name, geometry.joints.size() - 1);  
    }
    
    // find our special joints
    geometry.leftEyeJointIndex = modelIDs.indexOf(jointEyeLeftID);
    geometry.rightEyeJointIndex = modelIDs.indexOf(jointEyeRightID);
    geometry.neckJointIndex = modelIDs.indexOf(jointNeckID);
    geometry.rootJointIndex = modelIDs.indexOf(jointRootID);
    geometry.leanJointIndex = modelIDs.indexOf(jointLeanID);
    geometry.headJointIndex = modelIDs.indexOf(jointHeadID);
    geometry.leftHandJointIndex = modelIDs.indexOf(jointLeftHandID);
    geometry.rightHandJointIndex = modelIDs.indexOf(jointRightHandID);
    
    // extract the translation component of the neck transform
    if (geometry.neckJointIndex != -1) {
        const glm::mat4& transform = geometry.joints.at(geometry.neckJointIndex).transform;
        geometry.neckPivot = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
    }
    
    QVariantHash springs = mapping.value("spring").toHash();
    QVariant defaultSpring = springs.value("default");
    for (QHash<QString, ExtractedMesh>::iterator it = meshes.begin(); it != meshes.end(); it++) {
        ExtractedMesh& extracted = it.value();
        
        // accumulate local transforms
        QString modelID = models.contains(it.key()) ? it.key() : parentMap.value(it.key());
        extracted.mesh.springiness = springs.value(models.value(modelID).name, defaultSpring).toFloat();
        glm::mat4 modelTransform = getGlobalTransform(parentMap, models, modelID);
        
        // look for textures, material properties
        int partIndex = extracted.mesh.parts.size() - 1;
        bool generateTangents = false;
        foreach (const QString& childID, childMap.values(modelID)) {
            if (partIndex < 0) {
                break;
            }
            FBXMeshPart& part = extracted.mesh.parts[partIndex];
            if (textureFilenames.contains(childID)) {
                part.diffuseFilename = textureFilenames.value(childID);
                continue;
            }
            if (!materials.contains(childID)) {
                continue;
            }
            Material material = materials.value(childID);
            part.diffuseColor = material.diffuse;
            part.specularColor = material.specular;
            part.shininess = material.shininess;
            QString diffuseTextureID = diffuseTextures.value(childID);
            if (!diffuseTextureID.isNull()) {
                part.diffuseFilename = textureFilenames.value(diffuseTextureID);
                
                // FBX files generated by 3DSMax have an intermediate texture parent, apparently
                foreach (const QString& childTextureID, childMap.values(diffuseTextureID)) {
                    if (textureFilenames.contains(childTextureID)) {
                        part.diffuseFilename = textureFilenames.value(childTextureID);
                    }
                }
            }
            QString bumpTextureID = bumpTextures.value(childID);
            if (!bumpTextureID.isNull()) {
                part.normalFilename = textureFilenames.value(bumpTextureID);
                generateTangents = true;
            }
            partIndex--;
        }
        
        // if we have a normal map (and texture coordinates), we must compute tangents
        if (generateTangents && !extracted.mesh.texCoords.isEmpty()) {
            extracted.mesh.tangents.resize(extracted.mesh.vertices.size());
            foreach (const FBXMeshPart& part, extracted.mesh.parts) {
                for (int i = 0; i < part.quadIndices.size(); i += 4) {
                    setTangents(extracted.mesh, part.quadIndices.at(i), part.quadIndices.at(i + 1));
                    setTangents(extracted.mesh, part.quadIndices.at(i + 1), part.quadIndices.at(i + 2));
                    setTangents(extracted.mesh, part.quadIndices.at(i + 2), part.quadIndices.at(i + 3));
                    setTangents(extracted.mesh, part.quadIndices.at(i + 3), part.quadIndices.at(i));
                }
                for (int i = 0; i < part.triangleIndices.size(); i += 3) {
                    setTangents(extracted.mesh, part.triangleIndices.at(i), part.triangleIndices.at(i + 1));
                    setTangents(extracted.mesh, part.triangleIndices.at(i + 1), part.triangleIndices.at(i + 2));
                    setTangents(extracted.mesh, part.triangleIndices.at(i + 2), part.triangleIndices.at(i));
                }
            }
        }
        
        // find the clusters with which the mesh is associated
        extracted.mesh.isEye = false;
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
                    extracted.mesh.isEye = true;
                }
                // see http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx for a discussion
                // of skinning information in FBX
                fbxCluster.jointIndex = modelIDs.indexOf(jointID);
                if (fbxCluster.jointIndex == -1) {
                    qDebug() << "Joint not in model list: " << jointID << "\n";
                    fbxCluster.jointIndex = 0;
                }
                fbxCluster.inverseBindMatrix = glm::inverse(cluster.transformLink) * modelTransform;
                extracted.mesh.clusters.append(fbxCluster);
            }
        }
        
        // if we don't have a skinned joint, parent to the model itself
        if (extracted.mesh.clusters.isEmpty()) {
            FBXCluster cluster;
            cluster.jointIndex = modelIDs.indexOf(modelID);
            if (cluster.jointIndex == -1) {
                qDebug() << "Model not in model list: " << modelID << "\n";
                cluster.jointIndex = 0;
            }
            extracted.mesh.clusters.append(cluster);
        }
        
        // whether we're skinned depends on how many clusters are attached
        if (clusterIDs.size() > 1) {
            extracted.mesh.clusterIndices.resize(extracted.mesh.vertices.size());
            extracted.mesh.clusterWeights.resize(extracted.mesh.vertices.size());
            for (int i = 0; i < clusterIDs.size(); i++) {
                QString clusterID = clusterIDs.at(i);
                const Cluster& cluster = clusters[clusterID];
                
                for (int j = 0; j < cluster.indices.size(); j++) {
                    int oldIndex = cluster.indices.at(j);
                    float weight = cluster.weights.at(j);
                    for (QMultiHash<int, int>::const_iterator it = extracted.newIndices.constFind(oldIndex);
                            it != extracted.newIndices.end() && it.key() == oldIndex; it++) {
                        glm::vec4& weights = extracted.mesh.clusterWeights[it.value()];
                    
                        // look for an unused slot in the weights vector
                        for (int k = 0; k < 4; k++) {
                            if (weights[k] == 0.0f) {
                                extracted.mesh.clusterIndices[it.value()][k] = i;
                                weights[k] = weight;
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        // extract spring edges, connections if springy
        if (extracted.mesh.springiness > 0.0f) {
            QSet<QPair<int, int> > edges;
            
            extracted.mesh.vertexConnections.resize(extracted.mesh.vertices.size());
            foreach (const FBXMeshPart& part, extracted.mesh.parts) {
                for (int i = 0; i < part.quadIndices.size(); i += 4) {
                    int index0 = part.quadIndices.at(i);
                    int index1 = part.quadIndices.at(i + 1);
                    int index2 = part.quadIndices.at(i + 2);
                    int index3 = part.quadIndices.at(i + 3);
                    
                    edges.insert(QPair<int, int>(qMin(index0, index1), qMax(index0, index1)));
                    edges.insert(QPair<int, int>(qMin(index1, index2), qMax(index1, index2)));
                    edges.insert(QPair<int, int>(qMin(index2, index3), qMax(index2, index3)));
                    edges.insert(QPair<int, int>(qMin(index3, index0), qMax(index3, index0)));
               
                    extracted.mesh.vertexConnections[index0].append(QPair<int, int>(index3, index1));
                    extracted.mesh.vertexConnections[index1].append(QPair<int, int>(index0, index2));
                    extracted.mesh.vertexConnections[index2].append(QPair<int, int>(index1, index3));
                    extracted.mesh.vertexConnections[index3].append(QPair<int, int>(index2, index0));
                }
                for (int i = 0; i < part.triangleIndices.size(); i += 3) {
                    int index0 = part.triangleIndices.at(i);
                    int index1 = part.triangleIndices.at(i + 1);
                    int index2 = part.triangleIndices.at(i + 2);
                    
                    edges.insert(QPair<int, int>(qMin(index0, index1), qMax(index0, index1)));
                    edges.insert(QPair<int, int>(qMin(index1, index2), qMax(index1, index2)));
                    edges.insert(QPair<int, int>(qMin(index2, index0), qMax(index2, index0)));
                    
                    extracted.mesh.vertexConnections[index0].append(QPair<int, int>(index2, index1));
                    extracted.mesh.vertexConnections[index1].append(QPair<int, int>(index0, index2));
                    extracted.mesh.vertexConnections[index2].append(QPair<int, int>(index1, index0));
                }
            }
            
            for (QSet<QPair<int, int> >::const_iterator edge = edges.constBegin(); edge != edges.constEnd(); edge++) {
                extracted.mesh.springEdges.append(*edge);
            }
        }
        
        geometry.meshes.append(extracted.mesh);
    }
    
    // process attachments
    QVariantHash attachments = mapping.value("attach").toHash();
    for (QVariantHash::const_iterator it = attachments.constBegin(); it != attachments.constEnd(); it++) {
        FBXAttachment attachment;
        attachment.jointIndex = modelIDs.indexOf(processID(it.key()));
        attachment.scale = glm::vec3(1.0f, 1.0f, 1.0f);
        
        QVariantList properties = it->toList();
        if (properties.isEmpty()) {
            attachment.url = it->toString();
        } else {
            attachment.url = properties.at(0).toString();
            
            if (properties.size() >= 2) {
                attachment.translation = parseVec3(properties.at(1).toString());
            
                if (properties.size() >= 3) {
                    attachment.rotation = glm::quat(glm::radians(parseVec3(properties.at(2).toString())));
                
                    if (properties.size() >= 4) {
                        attachment.scale = parseVec3(properties.at(3).toString());
                    }
                }
            }
        }
        geometry.attachments.append(attachment);
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

bool addMeshVoxelsOperation(VoxelNode* node, void* extraData) {
    if (!node->isLeaf()) {
        return true;
    }
    FBXMesh& mesh = *static_cast<FBXMesh*>(extraData);
    FBXMeshPart& part = mesh.parts[0];
    
    const int FACE_COUNT = 6;
    const int VERTICES_PER_FACE = 4;
    const int VERTEX_COUNT = FACE_COUNT * VERTICES_PER_FACE;
    const float EIGHT_BIT_MAXIMUM = 255.0f;
    glm::vec3 color = glm::vec3(node->getColor()[0], node->getColor()[1], node->getColor()[2]) / EIGHT_BIT_MAXIMUM;
    for (int i = 0; i < VERTEX_COUNT; i++) {
        part.quadIndices.append(part.quadIndices.size());
        mesh.colors.append(color);
    }
    glm::vec3 corner = node->getCorner();
    float scale = node->getScale();
    
    mesh.vertices.append(glm::vec3(corner.x, corner.y, corner.z));
    mesh.vertices.append(glm::vec3(corner.x, corner.y, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x, corner.y + scale, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x, corner.y + scale, corner.z));
    for (int i = 0; i < VERTICES_PER_FACE; i++) {
        mesh.normals.append(glm::vec3(-1.0f, 0.0f, 0.0f));
    }
    
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y, corner.z));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y + scale, corner.z));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y + scale, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y, corner.z + scale));
    for (int i = 0; i < VERTICES_PER_FACE; i++) {
        mesh.normals.append(glm::vec3(1.0f, 0.0f, 0.0f));
    }
    
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y, corner.z));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x, corner.y, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x, corner.y, corner.z));
    for (int i = 0; i < VERTICES_PER_FACE; i++) {
        mesh.normals.append(glm::vec3(0.0f, -1.0f, 0.0f));
    }
    
    mesh.vertices.append(glm::vec3(corner.x, corner.y + scale, corner.z));
    mesh.vertices.append(glm::vec3(corner.x, corner.y + scale, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y + scale, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y + scale, corner.z));
    for (int i = 0; i < VERTICES_PER_FACE; i++) {
        mesh.normals.append(glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    mesh.vertices.append(glm::vec3(corner.x, corner.y + scale, corner.z));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y + scale, corner.z));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y, corner.z));
    mesh.vertices.append(glm::vec3(corner.x, corner.y, corner.z));
    for (int i = 0; i < VERTICES_PER_FACE; i++) {
        mesh.normals.append(glm::vec3(0.0f, 0.0f, -1.0f));
    }

    mesh.vertices.append(glm::vec3(corner.x, corner.y, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x + scale, corner.y + scale, corner.z + scale));
    mesh.vertices.append(glm::vec3(corner.x, corner.y + scale, corner.z + scale));
    for (int i = 0; i < VERTICES_PER_FACE; i++) {
        mesh.normals.append(glm::vec3(0.0f, 0.0f, 1.0f));
    }       
    
    return true;
}

FBXGeometry readSVO(const QByteArray& model) {
    FBXGeometry geometry;
    
    // we have one joint
    FBXJoint joint = { false };
    joint.parentIndex = -1;
    geometry.joints.append(joint);
    
    // and one mesh with one cluster and one part
    FBXMesh mesh;
    mesh.isEye = false;
    mesh.springiness = 0.0f;
    
    FBXCluster cluster = { 0 };
    mesh.clusters.append(cluster);
    
    FBXMeshPart part;
    part.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    part.shininess = 96.0f;
    mesh.parts.append(part);
    
    VoxelTree tree;
    ReadBitstreamToTreeParams args(WANT_COLOR, NO_EXISTS_BITS);
    tree.readBitstreamToTree((unsigned char*)model.data(), model.size(), args);
    tree.recurseTreeWithOperation(addMeshVoxelsOperation, &mesh);
    
    geometry.meshes.append(mesh);
    
    return geometry;
}
