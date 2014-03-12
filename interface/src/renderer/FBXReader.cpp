//
//  FBXReader.cpp
//  interface
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include <iostream>
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

#include <GeometryUtil.h>
#include <Shape.h>
#include <VoxelTree.h>

#include "FBXReader.h"
#include "Util.h"

using namespace std;

void Extents::reset() {
    minimum = glm::vec3(FLT_MAX);
    maximum = glm::vec3(-FLT_MAX);
}

bool Extents::containsPoint(const glm::vec3& point) const {
    return (point.x >= minimum.x && point.x <= maximum.x
        && point.y >= minimum.y && point.y <= maximum.y
        && point.z >= minimum.z && point.z <= maximum.z);
}

void Extents::addPoint(const glm::vec3& point) {
    minimum = glm::min(minimum, point);
    maximum = glm::max(maximum, point);
}

QStringList FBXGeometry::getJointNames() const {
    QStringList names;
    foreach (const FBXJoint& joint, joints) {
        names.append(joint.name);
    }
    return names;
}

static int fbxGeometryMetaTypeId = qRegisterMetaType<FBXGeometry>();

template<class T> QVariant readBinaryArray(QDataStream& in) {
    quint32 arrayLength;
    quint32 encoding;
    quint32 compressedLength;

    in >> arrayLength;
    in >> encoding;
    in >> compressedLength;

    QVector<T> values;
    const unsigned int DEFLATE_ENCODING = 1;
    if (encoding == DEFLATE_ENCODING) {
        // preface encoded data with uncompressed length
        QByteArray compressed(sizeof(quint32) + compressedLength, 0);
        *((quint32*)compressed.data()) = qToBigEndian<quint32>(arrayLength * sizeof(T));
        in.readRawData(compressed.data() + sizeof(quint32), compressedLength);
        QByteArray uncompressed = qUncompress(compressed);
        QDataStream uncompressedIn(uncompressed);
        uncompressedIn.setByteOrder(QDataStream::LittleEndian);
        uncompressedIn.setVersion(QDataStream::Qt_4_5); // for single/double precision switch
        for (quint32 i = 0; i < arrayLength; i++) {
            T value;
            uncompressedIn >> value;
            values.append(value);
        }
    } else {
        for (quint32 i = 0; i < arrayLength; i++) {
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
    const unsigned int MIN_VALID_OFFSET = 40;
    if (endOffset < MIN_VALID_OFFSET || nameLength == 0) {
        // use a null name to indicate a null node
        return node;
    }
    node.name = in.device()->read(nameLength);

    for (quint32 i = 0; i < propertyCount; i++) {
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
    void ungetChar(char ch) { _device->ungetChar(ch); }

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
            QByteArray datum = tokenizer.getDatum();
            if ((token = tokenizer.nextToken()) == ':') {
                tokenizer.ungetChar(':');
                tokenizer.pushBackToken(Tokenizer::DATUM_TOKEN);
                return node;    
                
            } else {
                tokenizer.pushBackToken(token);
                node.properties.append(datum);
                expectingDatum = false;
            }
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
    if (index >= properties.size()) {
        return QVector<int>();
    }
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
    if (index >= properties.size()) {
        return QVector<double>();
    }
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
    glm::vec3 translation;
    glm::mat4 preTransform;
    glm::quat preRotation;
    glm::quat rotation;
    glm::quat postRotation;
    glm::mat4 postTransform;

    glm::vec3 rotationMin;  // radians
    glm::vec3 rotationMax;  // radians
};

glm::mat4 getGlobalTransform(const QMultiHash<QString, QString>& parentMap,
        const QHash<QString, FBXModel>& models, QString nodeID) {
    glm::mat4 globalTransform;
    while (!nodeID.isNull()) {
        const FBXModel& model = models.value(nodeID);
        globalTransform = glm::translate(model.translation) * model.preTransform * glm::mat4_cast(model.preRotation *
            model.rotation * model.postRotation) * model.postTransform * globalTransform;

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

void printNode(const FBXNode& node, int indentLevel) {
    int indentLength = 2;
    QByteArray spaces(indentLevel * indentLength, ' ');
    QDebug nodeDebug = qDebug();
    
    nodeDebug.nospace() << spaces.data() << node.name.data() << ": ";
    foreach (const QVariant& property, node.properties) {
        nodeDebug << property;
    }
    
    foreach (const FBXNode& child, node.children) {
        printNode(child, indentLevel + 1);
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
        QHash<QString, FBXModel>& models, QSet<QString>& remainingModels, QVector<QString>& modelIDs) {
    if (remainingModels.contains(parentID)) {
        modelIDs.append(parentID);
        remainingModels.remove(parentID);
    }
    int parentIndex = modelIDs.size() - 1;
    foreach (const QString& childID, childMap.values(parentID)) {
        if (remainingModels.contains(childID)) {
            FBXModel& model = models[childID];
            if (model.parentIndex == -1) {
                model.parentIndex = parentIndex;
                appendModelIDs(childID, childMap, models, remainingModels, modelIDs);
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
    QVector<QHash<int, int> > blendshapeIndexMaps;
    QVector<QPair<int, int> > partMaterialTextures;
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
    QVector<int> textures;
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
        } else if (child.name == "LayerElementTexture") {
            foreach (const FBXNode& subdata, child.children) {
                if (subdata.name == "TextureId") {
                    textures = getIntVector(subdata.properties, 0);
                }
            }
        }
    }

    // convert the polygons to quads and triangles
    int polygonIndex = 0;
    QHash<QPair<int, int>, int> materialTextureParts;
    for (int beginIndex = 0; beginIndex < data.polygonIndices.size(); polygonIndex++) {
        int endIndex = beginIndex;
        while (data.polygonIndices.at(endIndex++) >= 0);

        QPair<int, int> materialTexture((polygonIndex < materials.size()) ? materials.at(polygonIndex) : 0,
            (polygonIndex < textures.size()) ? textures.at(polygonIndex) : 0);
        int& partIndex = materialTextureParts[materialTexture];
        if (partIndex == 0) {
            data.extracted.partMaterialTextures.append(materialTexture);
            data.extracted.mesh.parts.resize(data.extracted.mesh.parts.size() + 1);
            partIndex = data.extracted.mesh.parts.size();
        }
        FBXMeshPart& part = data.extracted.mesh.parts[partIndex - 1];
        
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

FBXBlendshape extractBlendshape(const FBXNode& object) {
    FBXBlendshape blendshape;
    foreach (const FBXNode& data, object.children) {
        if (data.name == "Indexes") {
            blendshape.indices = getIntVector(data.properties, 0);

        } else if (data.name == "Vertices") {
            blendshape.vertices = createVec3Vector(getDoubleVector(data.properties, 0));

        } else if (data.name == "Normals") {
            blendshape.normals = createVec3Vector(getDoubleVector(data.properties, 0));
        }
    }
    return blendshape;
}

void setTangents(FBXMesh& mesh, int firstIndex, int secondIndex) {
    glm::vec3 normal = glm::normalize(mesh.normals.at(firstIndex));
    glm::vec3 bitangent = glm::cross(normal, mesh.vertices.at(secondIndex) - mesh.vertices.at(firstIndex));
    if (glm::length(bitangent) < EPSILON) {
        return;
    }
    glm::vec2 texCoordDelta = mesh.texCoords.at(secondIndex) - mesh.texCoords.at(firstIndex);
    mesh.tangents[firstIndex] += glm::cross(glm::angleAxis(
                - atan2f(-texCoordDelta.t, texCoordDelta.s), normal) * glm::normalize(bitangent), normal);
}

QVector<int> getIndices(const QVector<QString> ids, QVector<QString> modelIDs) {
    QVector<int> indices;
    foreach (const QString& id, ids) {
        int index = modelIDs.indexOf(id);
        if (index != -1) {
            indices.append(index);
        }
    }
    return indices;
}

typedef QPair<int, float> WeightedIndex;

void addBlendshapes(const ExtractedBlendshape& extracted, const QList<WeightedIndex>& indices, ExtractedMesh& extractedMesh) {
    foreach (const WeightedIndex& index, indices) {
        extractedMesh.mesh.blendshapes.resize(max(extractedMesh.mesh.blendshapes.size(), index.first + 1));
        extractedMesh.blendshapeIndexMaps.resize(extractedMesh.mesh.blendshapes.size());
        FBXBlendshape& blendshape = extractedMesh.mesh.blendshapes[index.first];
        QHash<int, int>& blendshapeIndexMap = extractedMesh.blendshapeIndexMaps[index.first];
        for (int i = 0; i < extracted.blendshape.indices.size(); i++) {
            int oldIndex = extracted.blendshape.indices.at(i);
            for (QMultiHash<int, int>::const_iterator it = extractedMesh.newIndices.constFind(oldIndex);
                    it != extractedMesh.newIndices.constEnd() && it.key() == oldIndex; it++) {
                QHash<int, int>::iterator blendshapeIndex = blendshapeIndexMap.find(it.value());
                if (blendshapeIndex == blendshapeIndexMap.end()) {
                    blendshapeIndexMap.insert(it.value(), blendshape.indices.size());
                    blendshape.indices.append(it.value());
                    blendshape.vertices.append(extracted.blendshape.vertices.at(i) * index.second);
                    blendshape.normals.append(extracted.blendshape.normals.at(i) * index.second);
                } else {
                    blendshape.vertices[*blendshapeIndex] += extracted.blendshape.vertices.at(i) * index.second;
                    blendshape.normals[*blendshapeIndex] += extracted.blendshape.normals.at(i) * index.second;
                }
            }
        }
    }
}

QString getTopModelID(const QMultiHash<QString, QString>& parentMap,
        const QHash<QString, FBXModel>& models, const QString& modelID) {
    QString topID = modelID;
    forever {
        foreach (const QString& parentID, parentMap.values(topID)) {
            if (models.contains(parentID)) {
                topID = parentID;
                goto outerContinue;
            }
        }
        return topID;
        
        outerContinue: ;
    }
}

QString getString(const QVariant& value) {
    // if it's a list, return the first entry
    QVariantList list = value.toList();
    return list.isEmpty() ? value.toString() : list.at(0).toString();
}

class JointShapeInfo {
public:
    JointShapeInfo() : numVertices(0), numProjectedVertices(0), averageVertex(0.f), boneBegin(0.f), averageRadius(0.f) {
        extents.reset();
    }

    // NOTE: the points here are in the "joint frame" which has the "jointEnd" at the origin
    int numVertices;            // num vertices from contributing meshes
    int numProjectedVertices;   // num vertices that successfully project onto bone axis
    Extents extents;            // max and min extents of mesh vertices (in joint frame)
    glm::vec3 averageVertex;    // average of all mesh vertices (in joint frame)
    glm::vec3 boneBegin;        // parent joint location (in joint frame)
    float averageRadius;        // average distance from mesh points to averageVertex
};

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
    QString jointEyeLeftName = processID(getString(joints.value("jointEyeLeft", "jointEyeLeft")));
    QString jointEyeRightName = processID(getString(joints.value("jointEyeRight", "jointEyeRight")));
    QString jointNeckName = processID(getString(joints.value("jointNeck", "jointNeck")));
    QString jointRootName = processID(getString(joints.value("jointRoot", "jointRoot")));
    QString jointLeanName = processID(getString(joints.value("jointLean", "jointLean")));
    QString jointHeadName = processID(getString(joints.value("jointHead", "jointHead")));
    QString jointLeftHandName = processID(getString(joints.value("jointLeftHand", "jointLeftHand")));
    QString jointRightHandName = processID(getString(joints.value("jointRightHand", "jointRightHand")));
    QVariantList jointLeftFingerNames = joints.values("jointLeftFinger");
    QVariantList jointRightFingerNames = joints.values("jointRightFinger");
    QVariantList jointLeftFingertipNames = joints.values("jointLeftFingertip");
    QVariantList jointRightFingertipNames = joints.values("jointRightFingertip");
    QString jointEyeLeftID;
    QString jointEyeRightID;
    QString jointNeckID;
    QString jointRootID;
    QString jointLeanID;
    QString jointHeadID;
    QString jointLeftHandID;
    QString jointRightHandID;
    QVector<QString> jointLeftFingerIDs(jointLeftFingerNames.size());
    QVector<QString> jointRightFingerIDs(jointRightFingerNames.size());
    QVector<QString> jointLeftFingertipIDs(jointLeftFingertipNames.size());
    QVector<QString> jointRightFingertipIDs(jointRightFingertipNames.size());

    QVariantHash blendshapeMappings = mapping.value("bs").toHash();
    QMultiHash<QByteArray, WeightedIndex> blendshapeIndices;
    for (int i = 0;; i++) {
        QByteArray blendshapeName = FACESHIFT_BLENDSHAPES[i];
        if (blendshapeName.isEmpty()) {
            break;
        }
        QList<QVariant> mappings = blendshapeMappings.values(blendshapeName);
        if (mappings.isEmpty()) {
            blendshapeIndices.insert(blendshapeName, WeightedIndex(i, 1.0f));
        } else {
            foreach (const QVariant& mapping, mappings) {
                QVariantList blendshapeMapping = mapping.toList();
                blendshapeIndices.insert(blendshapeMapping.at(0).toByteArray(),
                   WeightedIndex(i, blendshapeMapping.at(1).toFloat()));
            }
        }
    }
    QMultiHash<QString, WeightedIndex> blendshapeChannelIndices;

    foreach (const FBXNode& child, node.children) {
        if (child.name == "Objects") {
            foreach (const FBXNode& object, child.children) {
                if (object.name == "Geometry") {
                    if (object.properties.at(2) == "Mesh") {
                        meshes.insert(getID(object.properties), extractMesh(object));

                    } else { // object.properties.at(2) == "Shape"
                        ExtractedBlendshape extracted = { getID(object.properties), extractBlendshape(object) };
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
                    int index;
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

                    } else if ((index = jointLeftFingerNames.indexOf(name)) != -1) {
                        jointLeftFingerIDs[index] = getID(object.properties);

                    } else if ((index = jointRightFingerNames.indexOf(name)) != -1) {
                        jointRightFingerIDs[index] = getID(object.properties);

                    } else if ((index = jointLeftFingertipNames.indexOf(name)) != -1) {
                        jointLeftFingertipIDs[index] = getID(object.properties);

                    } else if ((index = jointRightFingertipNames.indexOf(name)) != -1) {
                        jointRightFingertipIDs[index] = getID(object.properties);
                    }
                    glm::vec3 translation;
                    // NOTE: the euler angles as supplied by the FBX file are in degrees
                    glm::vec3 rotationOffset;
                    glm::vec3 preRotation, rotation, postRotation;
                    glm::vec3 scale = glm::vec3(1.0f, 1.0f, 1.0f);
                    glm::vec3 scalePivot, rotationPivot;
                    bool rotationMinX = false, rotationMinY = false, rotationMinZ = false;
                    bool rotationMaxX = false, rotationMaxY = false, rotationMaxZ = false;
                    glm::vec3 rotationMin, rotationMax;
                    FBXModel model = { name, -1 };
                    ExtractedMesh* mesh = NULL;
                    QVector<ExtractedBlendshape> blendshapes;
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

                                    } else if (property.properties.at(0) == "RotationMin") {
                                        rotationMin = getVec3(property.properties, index);

                                    } 
                                    // NOTE: these rotation limits are stored in degrees (NOT radians)
                                    else if (property.properties.at(0) == "RotationMax") {
                                        rotationMax = getVec3(property.properties, index);

                                    } else if (property.properties.at(0) == "RotationMinX") {
                                        rotationMinX = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMinY") {
                                        rotationMinY = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMinZ") {
                                        rotationMinZ = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMaxX") {
                                        rotationMaxX = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMaxY") {
                                        rotationMaxY = property.properties.at(index).toBool();

                                    } else if (property.properties.at(0) == "RotationMaxZ") {
                                        rotationMaxZ = property.properties.at(index).toBool();
                                    }
                                }
                            }
                        } else if (subobject.name == "Vertices") {
                            // it's a mesh as well as a model
                            mesh = &meshes[getID(object.properties)];
                            *mesh = extractMesh(object);
                             
                        } else if (subobject.name == "Shape") {
                            ExtractedBlendshape blendshape =  { subobject.properties.at(0).toString(),
                                extractBlendshape(subobject) };
                            blendshapes.append(blendshape);
                        }
                    }
                    
                    // add the blendshapes included in the model, if any
                    if (mesh) {
                        foreach (const ExtractedBlendshape& extracted, blendshapes) {
                            addBlendshapes(extracted, blendshapeIndices.values(extracted.id.toLatin1()), *mesh);
                        }
                    }
                    
                    // see FBX documentation, http://download.autodesk.com/us/fbx/20112/FBX_SDK_HELP/index.html
                    model.translation = translation;
                    model.preTransform = glm::translate(rotationOffset) * glm::translate(rotationPivot);      
                    model.preRotation = glm::quat(glm::radians(preRotation));            
                    model.rotation = glm::quat(glm::radians(rotation));
                    model.postRotation = glm::quat(glm::radians(postRotation));
                    model.postTransform = glm::translate(-rotationPivot) * glm::translate(scalePivot) *
                        glm::scale(scale) * glm::translate(-scalePivot);
                    // NOTE: anbgles from the FBX file are in degrees
                    // so we convert them to radians for the FBXModel class
                    model.rotationMin = glm::radians(glm::vec3(rotationMinX ? rotationMin.x : -180.0f,
                        rotationMinY ? rotationMin.y : -180.0f, rotationMinZ ? rotationMin.z : -180.0f));
                    model.rotationMax = glm::radians(glm::vec3(rotationMaxX ? rotationMax.x : 180.0f,
                        rotationMaxY ? rotationMax.y : 180.0f, rotationMaxZ ? rotationMax.z : 180.0f));
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
                        QString id = getID(object.properties);
                        foreach (const WeightedIndex& index, blendshapeIndices.values(name)) {
                            blendshapeChannelIndices.insert(id, index);
                        }
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
        QString blendshapeID = parentMap.value(blendshapeChannelID);
        QString meshID = parentMap.value(blendshapeID);
        addBlendshapes(extracted, blendshapeChannelIndices.values(blendshapeChannelID), meshes[meshID]);
    }

    // get offset transform from mapping
    FBXGeometry geometry;
    float offsetScale = mapping.value("scale", 1.0f).toFloat();
    glm::quat offsetRotation = glm::quat(glm::radians(glm::vec3(mapping.value("rx").toFloat(),
            mapping.value("ry").toFloat(), mapping.value("rz").toFloat())));
    geometry.offset = glm::translate(glm::vec3(mapping.value("tx").toFloat(), mapping.value("ty").toFloat(),
                                               mapping.value("tz").toFloat())) * glm::mat4_cast(offsetRotation) * glm::scale(glm::vec3(offsetScale, offsetScale, offsetScale));

    // get the list of models in depth-first traversal order
    QVector<QString> modelIDs;
    QSet<QString> remainingModels;
    for (QHash<QString, FBXModel>::const_iterator model = models.constBegin(); model != models.constEnd(); model++) {
        // models with clusters must be parented to the cluster top
        foreach (const QString& deformerID, childMap.values(model.key())) {
            foreach (const QString& clusterID, childMap.values(deformerID)) {
                if (!clusters.contains(clusterID)) {
                    continue;
                }
                QString topID = getTopModelID(parentMap, models, childMap.value(clusterID));
                childMap.remove(parentMap.take(model.key()), model.key());
                parentMap.insert(model.key(), topID);
                goto outerBreak;
            }
        }
        outerBreak:
    
        // make sure the parent is in the child map
        QString parent = parentMap.value(model.key());
        if (!childMap.contains(parent, model.key())) {
            childMap.insert(parent, model.key());
        }
        remainingModels.insert(model.key());
    }
    while (!remainingModels.isEmpty()) {
        QString topID = getTopModelID(parentMap, models, *remainingModels.constBegin());
        appendModelIDs(parentMap.value(topID), childMap, models, remainingModels, modelIDs);
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
        joint.translation = model.translation;
        joint.preTransform = model.preTransform;
        joint.preRotation = model.preRotation;
        joint.rotation = model.rotation;
        joint.postRotation = model.postRotation;
        joint.postTransform = model.postTransform;
        joint.rotationMin = model.rotationMin;
        joint.rotationMax = model.rotationMax;
        glm::quat combinedRotation = model.preRotation * model.rotation * model.postRotation;
        if (joint.parentIndex == -1) {
            joint.transform = geometry.offset * glm::translate(model.translation) * model.preTransform * 
                glm::mat4_cast(combinedRotation) * model.postTransform;
            joint.inverseDefaultRotation = glm::inverse(combinedRotation);
            joint.distanceToParent = 0.0f;

        } else {
            const FBXJoint& parentJoint = geometry.joints.at(joint.parentIndex);
            joint.transform = parentJoint.transform * glm::translate(model.translation) *
                model.preTransform * glm::mat4_cast(combinedRotation) * model.postTransform;
            joint.inverseDefaultRotation = glm::inverse(combinedRotation) * parentJoint.inverseDefaultRotation;
            joint.distanceToParent = glm::distance(extractTranslation(parentJoint.transform),
                extractTranslation(joint.transform));
        }
        joint.boneRadius = 0.0f;
        joint.inverseBindRotation = joint.inverseDefaultRotation;
        joint.name = model.name;
        joint.shapePosition = glm::vec3(0.f);
        joint.shapeType = Shape::UNKNOWN_SHAPE;
        geometry.joints.append(joint);
        geometry.jointIndices.insert(model.name, geometry.joints.size());
    }
    // for each joint we allocate a JointShapeInfo in which we'll store collision shape info
    QVector<JointShapeInfo> jointShapeInfos;
    jointShapeInfos.resize(geometry.joints.size());

    // find our special joints
    geometry.leftEyeJointIndex = modelIDs.indexOf(jointEyeLeftID);
    geometry.rightEyeJointIndex = modelIDs.indexOf(jointEyeRightID);
    geometry.neckJointIndex = modelIDs.indexOf(jointNeckID);
    geometry.rootJointIndex = modelIDs.indexOf(jointRootID);
    geometry.leanJointIndex = modelIDs.indexOf(jointLeanID);
    geometry.headJointIndex = modelIDs.indexOf(jointHeadID);
    geometry.leftHandJointIndex = modelIDs.indexOf(jointLeftHandID);
    geometry.rightHandJointIndex = modelIDs.indexOf(jointRightHandID);
    geometry.leftFingerJointIndices = getIndices(jointLeftFingerIDs, modelIDs);
    geometry.rightFingerJointIndices = getIndices(jointRightFingerIDs, modelIDs);
    geometry.leftFingertipJointIndices = getIndices(jointLeftFingertipIDs, modelIDs);
    geometry.rightFingertipJointIndices = getIndices(jointRightFingertipIDs, modelIDs);

    // extract the translation component of the neck transform
    if (geometry.neckJointIndex != -1) {
        const glm::mat4& transform = geometry.joints.at(geometry.neckJointIndex).transform;
        geometry.neckPivot = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
    }

    geometry.bindExtents.reset();
    geometry.staticExtents.reset();
    geometry.meshExtents.reset();
    
    QVariantHash springs = mapping.value("spring").toHash();
    QVariant defaultSpring = springs.value("default");
    for (QHash<QString, ExtractedMesh>::iterator it = meshes.begin(); it != meshes.end(); it++) {
        ExtractedMesh& extracted = it.value();

        // accumulate local transforms
        QString modelID = models.contains(it.key()) ? it.key() : parentMap.value(it.key());
        extracted.mesh.springiness = springs.value(models.value(modelID).name, defaultSpring).toFloat();
        glm::mat4 modelTransform = getGlobalTransform(parentMap, models, modelID);

        // compute the mesh extents from the transformed vertices
        foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
            glm::vec3 transformedVertex = glm::vec3(modelTransform * glm::vec4(vertex, 1.0f));
            geometry.meshExtents.minimum = glm::min(geometry.meshExtents.minimum, transformedVertex);
            geometry.meshExtents.maximum = glm::max(geometry.meshExtents.maximum, transformedVertex);
        }

        // look for textures, material properties
        int materialIndex = 0;
        int textureIndex = 0;
        bool generateTangents = false;
        QList<QString> children = childMap.values(modelID);
        for (int i = children.size() - 1; i >= 0; i--) {
            const QString& childID = children.at(i);
            if (materials.contains(childID)) {
                Material material = materials.value(childID);
                
                QByteArray diffuseFilename;
                QString diffuseTextureID = diffuseTextures.value(childID);
                if (!diffuseTextureID.isNull()) {
                    diffuseFilename = textureFilenames.value(diffuseTextureID);

                    // FBX files generated by 3DSMax have an intermediate texture parent, apparently
                    foreach (const QString& childTextureID, childMap.values(diffuseTextureID)) {
                        if (textureFilenames.contains(childTextureID)) {
                            diffuseFilename = textureFilenames.value(childTextureID);
                        }
                    }
                }
                
                QByteArray normalFilename;
                QString bumpTextureID = bumpTextures.value(childID);
                if (!bumpTextureID.isNull()) {
                    normalFilename = textureFilenames.value(bumpTextureID);
                    generateTangents = true;
                }
                
                for (int j = 0; j < extracted.partMaterialTextures.size(); j++) {
                    if (extracted.partMaterialTextures.at(j).first == materialIndex) {
                        FBXMeshPart& part = extracted.mesh.parts[j];
                        part.diffuseColor = material.diffuse;
                        part.specularColor = material.specular;
                        part.shininess = material.shininess;
                        if (!diffuseFilename.isNull()) {
                            part.diffuseFilename = diffuseFilename;
                        }
                        if (!normalFilename.isNull()) {
                            part.normalFilename = normalFilename;
                        }
                    }
                }
                materialIndex++;
                
            } else if (textureFilenames.contains(childID)) {
                QByteArray filename = textureFilenames.value(childID);
                for (int j = 0; j < extracted.partMaterialTextures.size(); j++) {
                    if (extracted.partMaterialTextures.at(j).second == textureIndex) {
                        extracted.mesh.parts[j].diffuseFilename = filename;
                    }
                }
                textureIndex++;
            }
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
        QVector<QString> clusterIDs;
        foreach (const QString& childID, childMap.values(it.key())) {
            foreach (const QString& clusterID, childMap.values(childID)) {
                if (!clusters.contains(clusterID)) {
                    continue;
                }
                FBXCluster fbxCluster;
                const Cluster& cluster = clusters[clusterID];
                clusterIDs.append(clusterID);

                // see http://stackoverflow.com/questions/13566608/loading-skinning-information-from-fbx for a discussion
                // of skinning information in FBX
                QString jointID = childMap.value(clusterID);
                fbxCluster.jointIndex = modelIDs.indexOf(jointID);
                if (fbxCluster.jointIndex == -1) {
                    qDebug() << "Joint not in model list: " << jointID;
                    fbxCluster.jointIndex = 0;
                }
                fbxCluster.inverseBindMatrix = glm::inverse(cluster.transformLink) * modelTransform;
                extracted.mesh.clusters.append(fbxCluster);

                // override the bind rotation with the transform link
                FBXJoint& joint = geometry.joints[fbxCluster.jointIndex];
                joint.inverseBindRotation = glm::inverse(extractRotation(cluster.transformLink));
                joint.bindTransform = cluster.transformLink;
                
                // update the bind pose extents
                glm::vec3 bindTranslation = extractTranslation(geometry.offset * joint.bindTransform);
                geometry.bindExtents.addPoint(bindTranslation);
            }
        }

        // if we don't have a skinned joint, parent to the model itself
        if (extracted.mesh.clusters.isEmpty()) {
            FBXCluster cluster;
            cluster.jointIndex = modelIDs.indexOf(modelID);
            if (cluster.jointIndex == -1) {
                qDebug() << "Model not in model list: " << modelID;
                cluster.jointIndex = 0;
            }
            extracted.mesh.clusters.append(cluster);
        }

        // whether we're skinned depends on how many clusters are attached
        const FBXCluster& firstFBXCluster = extracted.mesh.clusters.at(0);
        int maxJointIndex = firstFBXCluster.jointIndex;
        glm::mat4 inverseModelTransform = glm::inverse(modelTransform);
        if (clusterIDs.size() > 1) {
            extracted.mesh.clusterIndices.resize(extracted.mesh.vertices.size());
            extracted.mesh.clusterWeights.resize(extracted.mesh.vertices.size());
            float maxWeight = 0.0f;
            for (int i = 0; i < clusterIDs.size(); i++) {
                QString clusterID = clusterIDs.at(i);
                const Cluster& cluster = clusters[clusterID];
                const FBXCluster& fbxCluster = extracted.mesh.clusters.at(i);
                int jointIndex = fbxCluster.jointIndex;
                FBXJoint& joint = geometry.joints[jointIndex];
                glm::mat4 transformJointToMesh = inverseModelTransform * joint.bindTransform;
                glm::quat rotateMeshToJoint = glm::inverse(extractRotation(transformJointToMesh));
                glm::vec3 boneEnd = extractTranslation(transformJointToMesh);
                glm::vec3 boneBegin = boneEnd;
                glm::vec3 boneDirection;
                float boneLength;
                if (joint.parentIndex != -1) {
                    boneBegin = extractTranslation(inverseModelTransform * geometry.joints[joint.parentIndex].bindTransform);
                    boneDirection = boneEnd - boneBegin;
                    boneLength = glm::length(boneDirection);
                    if (boneLength > EPSILON) {
                        boneDirection /= boneLength;
                    }
                }
                float radiusScale = extractUniformScale(joint.transform * fbxCluster.inverseBindMatrix);
                JointShapeInfo& jointShapeInfo = jointShapeInfos[jointIndex];
                jointShapeInfo.boneBegin = rotateMeshToJoint * (radiusScale * (boneBegin - boneEnd));

                bool jointIsStatic = joint.freeLineage.isEmpty();
                glm::vec3 jointTranslation = extractTranslation(geometry.offset * joint.bindTransform);
                float totalWeight = 0.0f;
                for (int j = 0; j < cluster.indices.size(); j++) {
                    int oldIndex = cluster.indices.at(j);
                    float weight = cluster.weights.at(j);
                    totalWeight += weight;
                    for (QMultiHash<int, int>::const_iterator it = extracted.newIndices.constFind(oldIndex);
                            it != extracted.newIndices.end() && it.key() == oldIndex; it++) {
                        // expand the bone radius for vertices with at least 1/4 weight
                        const float EXPANSION_WEIGHT_THRESHOLD = 0.25f;
                        if (weight > EXPANSION_WEIGHT_THRESHOLD) {
                            const glm::vec3& vertex = extracted.mesh.vertices.at(it.value());
                            float proj = glm::dot(boneDirection, vertex - boneEnd);
                            if (proj < 0.0f && proj > -boneLength) {
                                joint.boneRadius = glm::max(joint.boneRadius, 
                                        radiusScale * glm::distance(vertex, boneEnd + boneDirection * proj));
                                ++jointShapeInfo.numProjectedVertices;
                            }
                            glm::vec3 vertexInJointFrame = rotateMeshToJoint * (radiusScale * (vertex - boneEnd));
                            jointShapeInfo.extents.addPoint(vertexInJointFrame);
                            jointShapeInfo.averageVertex += vertexInJointFrame;
                            ++jointShapeInfo.numVertices;
                            if (jointIsStatic) {
                                // expand the extents of static (nonmovable) joints
                                geometry.staticExtents.addPoint(vertex + jointTranslation);
                            }
                        }

                        // look for an unused slot in the weights vector
                        glm::vec4& weights = extracted.mesh.clusterWeights[it.value()];
                        for (int k = 0; k < 4; k++) {
                            if (weights[k] == 0.0f) {
                                extracted.mesh.clusterIndices[it.value()][k] = i;
                                weights[k] = weight;
                                break;
                            }
                        }
                    }
                }
                if (totalWeight > maxWeight) {
                    maxWeight = totalWeight;
                    maxJointIndex = jointIndex;
                }
            }
        } else {
            int jointIndex = maxJointIndex;
            FBXJoint& joint = geometry.joints[jointIndex];
            JointShapeInfo& jointShapeInfo = jointShapeInfos[jointIndex];

            glm::mat4 transformJointToMesh = inverseModelTransform * joint.bindTransform;
            glm::quat rotateMeshToJoint = glm::inverse(extractRotation(transformJointToMesh));
            glm::vec3 boneEnd = extractTranslation(transformJointToMesh);
            glm::vec3 boneBegin = boneEnd;

            glm::vec3 boneDirection;
            float boneLength;
            if (joint.parentIndex != -1) {
                boneBegin = extractTranslation(inverseModelTransform * geometry.joints[joint.parentIndex].bindTransform);
                boneDirection = boneEnd - boneBegin;
                boneLength = glm::length(boneDirection);
                if (boneLength > EPSILON) {
                    boneDirection /= boneLength;
                }
            }
            float radiusScale = extractUniformScale(joint.transform * firstFBXCluster.inverseBindMatrix);
            jointShapeInfo.boneBegin = rotateMeshToJoint * (radiusScale * (boneBegin - boneEnd));

            glm::vec3 averageVertex(0.f);
            foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
                float proj = glm::dot(boneDirection, vertex - boneEnd);
                if (proj < 0.0f && proj > -boneLength) {
                    joint.boneRadius = glm::max(joint.boneRadius, radiusScale * glm::distance(vertex, boneEnd + boneDirection * proj));
                    ++jointShapeInfo.numProjectedVertices;
                }
                glm::vec3 vertexInJointFrame = rotateMeshToJoint * (radiusScale * (vertex - boneEnd));
                jointShapeInfo.extents.addPoint(vertexInJointFrame);
                jointShapeInfo.averageVertex += vertexInJointFrame;
                averageVertex += vertex;
            }
            int numVertices = extracted.mesh.vertices.size();
            jointShapeInfo.numVertices = numVertices;
            if (numVertices > 0) {
                averageVertex /= float(jointShapeInfo.numVertices);
                float averageRadius = 0.f;
                foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
                    averageRadius += glm::distance(vertex, averageVertex);
                }
                jointShapeInfo.averageRadius = averageRadius * radiusScale;
            }
        }
        extracted.mesh.isEye = (maxJointIndex == geometry.leftEyeJointIndex || maxJointIndex == geometry.rightEyeJointIndex);

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

    // now that all joints have been scanned, compute a collision shape for each joint
    glm::vec3 defaultCapsuleAxis(0.f, 1.f, 0.f);
    for (int i = 0; i < geometry.joints.size(); ++i) {
        FBXJoint& joint = geometry.joints[i];
        JointShapeInfo& jointShapeInfo = jointShapeInfos[i];

        // we use a capsule if the joint ANY mesh vertices successfully projected onto the bone
        // AND its boneRadius is not too close to zero
        bool collideLikeCapsule = jointShapeInfo.numProjectedVertices > 0
                && glm::length(jointShapeInfo.boneBegin) > EPSILON;

        if (collideLikeCapsule) {
            joint.shapeRotation = rotationBetween(defaultCapsuleAxis, jointShapeInfo.boneBegin);
            joint.shapePosition = 0.5f * jointShapeInfo.boneBegin;
            joint.shapeType = Shape::CAPSULE_SHAPE;
        } else {
            // collide the joint like a sphere
            if (jointShapeInfo.numVertices > 0) {
                jointShapeInfo.averageVertex /= float(jointShapeInfo.numVertices);
                joint.shapePosition = jointShapeInfo.averageVertex;
            } else {
                joint.shapePosition = glm::vec3(0.f);
                joint.shapeType = Shape::SPHERE_SHAPE;
            }
            if (jointShapeInfo.numProjectedVertices == 0
                   && jointShapeInfo.numVertices > 0) {
                // the bone projection algorithm was not able to compute the joint radius
                // so we use an alternative measure
                jointShapeInfo.averageRadius /= float(jointShapeInfo.numVertices);
                joint.boneRadius = jointShapeInfo.averageRadius;
            }
        }
    }
    geometry.palmDirection = parseVec3(mapping.value("palmDirection", "0, -1, 0").toString());

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

QVariantHash readMapping(const QByteArray& data) {
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    return parseMapping(&buffer);
}

FBXGeometry readFBX(const QByteArray& model, const QVariantHash& mapping) {
    QBuffer buffer(const_cast<QByteArray*>(&model));
    buffer.open(QIODevice::ReadOnly);
    return extractFBXGeometry(parseFBX(&buffer), mapping);
}

bool addMeshVoxelsOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = (VoxelTreeElement*)element;
    if (!voxel->isLeaf()) {
        return true;
    }
    FBXMesh& mesh = *static_cast<FBXMesh*>(extraData);
    FBXMeshPart& part = mesh.parts[0];

    const int FACE_COUNT = 6;
    const int VERTICES_PER_FACE = 4;
    const int VERTEX_COUNT = FACE_COUNT * VERTICES_PER_FACE;
    const float EIGHT_BIT_MAXIMUM = 255.0f;
    glm::vec3 color = glm::vec3(voxel->getColor()[0], voxel->getColor()[1], voxel->getColor()[2]) / EIGHT_BIT_MAXIMUM;
    for (int i = 0; i < VERTEX_COUNT; i++) {
        part.quadIndices.append(part.quadIndices.size());
        mesh.colors.append(color);
    }
    glm::vec3 corner = voxel->getCorner();
    float scale = voxel->getScale();

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

    unsigned char* dataAt = (unsigned char*)model.data();
    size_t dataSize = model.size();

    if (tree.getWantSVOfileVersions()) {
        // skip the type/version
        dataAt += sizeof(PacketType);
        dataSize -= sizeof(PacketType);
        dataAt += sizeof(PacketVersion);
        dataSize -= sizeof(PacketVersion);
    }   
    tree.readBitstreamToTree(dataAt, dataSize, args);
    tree.recurseTreeWithOperation(addMeshVoxelsOperation, &mesh);

    geometry.meshes.append(mesh);

    geometry.meshExtents.maximum = glm::vec3(1.0f, 1.0f, 1.0f);

    return geometry;
}
