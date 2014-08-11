//
//  FBXReader.cpp
//  interface/src/renderer
//
//  Created by Andrzej Kapolka on 9/18/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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

#include <GeometryUtil.h>
#include <GLMHelpers.h>
#include <OctalCode.h>
#include <Shape.h>

#include <VoxelTree.h>

#include "FBXReader.h"

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

void Extents::addExtents(const Extents& extents) {
     minimum = glm::min(minimum, extents.minimum);
     maximum = glm::max(maximum, extents.maximum);
}

void Extents::addPoint(const glm::vec3& point) {
    minimum = glm::min(minimum, point);
    maximum = glm::max(maximum, point);
}

bool FBXMesh::hasSpecularTexture() const {
    foreach (const FBXMeshPart& part, parts) {
        if (!part.specularTexture.filename.isEmpty()) {
            return true;
        }
    }
    return false;
}

QStringList FBXGeometry::getJointNames() const {
    QStringList names;
    foreach (const FBXJoint& joint, joints) {
        names.append(joint.name);
    }
    return names;
}

bool FBXGeometry::hasBlendedMeshes() const {
    foreach (const FBXMesh& mesh, meshes) {
        if (!mesh.blendshapes.isEmpty()) {
            return true;
        }
    }
    return false;
}

static int fbxGeometryMetaTypeId = qRegisterMetaType<FBXGeometry>();
static int fbxAnimationFrameMetaTypeId = qRegisterMetaType<FBXAnimationFrame>();
static int fbxAnimationFrameVectorMetaTypeId = qRegisterMetaType<QVector<FBXAnimationFrame> >();

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
    for (const double* it = doubleVector.constData(), *end = it + (doubleVector.size() / 3 * 3); it != end; ) {
        float x = *it++;
        float y = *it++;
        float z = *it++;
        values.append(glm::vec3(x, y, z));
    }
    return values;
}

QVector<glm::vec2> createVec2Vector(const QVector<double>& doubleVector) {
    QVector<glm::vec2> values;
    for (const double* it = doubleVector.constData(), *end = it + (doubleVector.size() / 2 * 2); it != end; ) {
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

QVector<int> getIntVector(const FBXNode& node) {
    foreach (const FBXNode& child, node.children) {
        if (child.name == "a") {
            return getIntVector(child);
        }
    }
    if (node.properties.isEmpty()) {
        return QVector<int>();
    }
    QVector<int> vector = node.properties.at(0).value<QVector<int> >();
    if (!vector.isEmpty()) {
        return vector;
    }
    for (int i = 0; i < node.properties.size(); i++) {
        vector.append(node.properties.at(i).toInt());
    }
    return vector;
}

QVector<float> getFloatVector(const FBXNode& node) {
    foreach (const FBXNode& child, node.children) {
        if (child.name == "a") {
            return getFloatVector(child);
        }
    }
    if (node.properties.isEmpty()) {
        return QVector<float>();
    }
    QVector<float> vector = node.properties.at(0).value<QVector<float> >();
    if (!vector.isEmpty()) {
        return vector;
    }
    for (int i = 0; i < node.properties.size(); i++) {
        vector.append(node.properties.at(i).toFloat());
    }
    return vector;
}

QVector<double> getDoubleVector(const FBXNode& node) {
    foreach (const FBXNode& child, node.children) {
        if (child.name == "a") {
            return getDoubleVector(child);
        }
    }
    if (node.properties.isEmpty()) {
        return QVector<double>();
    }
    QVector<double> vector = node.properties.at(0).value<QVector<double> >();
    if (!vector.isEmpty()) {
        return vector;
    }
    for (int i = 0; i < node.properties.size(); i++) {
        vector.append(node.properties.at(i).toDouble());
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
    return id.mid(id.lastIndexOf(':') + 1);
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

const int NUM_FACESHIFT_BLENDSHAPES = sizeof(FACESHIFT_BLENDSHAPES) / sizeof(char*);

const char* HUMANIK_JOINTS[] = {
    "RightHand",
    "RightForeArm",
    "RightArm",
    "Head",
    "LeftArm",
    "LeftForeArm",
    "LeftHand",
    "Neck",
    "Spine",
    "Hips",
    "RightUpLeg",
    "LeftUpLeg",
    "RightLeg",
    "LeftLeg",
    "RightFoot",
    "LeftFoot",
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
        const QHash<QString, FBXModel>& models, QString nodeID, bool mixamoHack) {
    glm::mat4 globalTransform;
    while (!nodeID.isNull()) {
        const FBXModel& model = models.value(nodeID);
        globalTransform = glm::translate(model.translation) * model.preTransform * glm::mat4_cast(model.preRotation *
            model.rotation * model.postRotation) * model.postTransform * globalTransform;
        if (mixamoHack) {
            // there's something weird about the models from Mixamo Fuse; they don't skin right with the full transform
            return globalTransform;
        }
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
    if (index >= data.polygonIndices.size()) {
        return;
    }
    int vertexIndex = data.polygonIndices.at(index);
    if (vertexIndex < 0) {
        vertexIndex = -vertexIndex - 1;
    }
    Vertex vertex;
    vertex.originalIndex = vertexIndex;
    
    glm::vec3 position;
    if (vertexIndex < data.vertices.size()) {
        position = data.vertices.at(vertexIndex);
    }

    glm::vec3 normal;
    int normalIndex = data.normalsByVertex ? vertexIndex : index;
    if (data.normalIndices.isEmpty()) {    
        if (normalIndex < data.normals.size()) {
            normal = data.normals.at(normalIndex);
        }
    } else if (normalIndex < data.normalIndices.size()) {
        normalIndex = data.normalIndices.at(normalIndex);
        if (normalIndex >= 0 && normalIndex < data.normals.size()) {
            normal = data.normals.at(normalIndex);
        }
    }

    if (data.texCoordIndices.isEmpty()) {
        if (index < data.texCoords.size()) {
            vertex.texCoord = data.texCoords.at(index);
        }
    } else if (index < data.texCoordIndices.size()) {
        int texCoordIndex = data.texCoordIndices.at(index);
        if (texCoordIndex >= 0 && texCoordIndex < data.texCoords.size()) {
            vertex.texCoord = data.texCoords.at(texCoordIndex);
        }
    }

    QHash<Vertex, int>::const_iterator it = data.indices.find(vertex);
    if (it == data.indices.constEnd()) {
        int newIndex = data.extracted.mesh.vertices.size();
        indices.append(newIndex);
        data.indices.insert(vertex, newIndex);
        data.extracted.newIndices.insert(vertexIndex, newIndex);
        data.extracted.mesh.vertices.append(position);
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
            data.vertices = createVec3Vector(getDoubleVector(child));

        } else if (child.name == "PolygonVertexIndex") {
            data.polygonIndices = getIntVector(child);

        } else if (child.name == "LayerElementNormal") {
            data.normalsByVertex = false;
            bool indexToDirect = false;
            foreach (const FBXNode& subdata, child.children) {
                if (subdata.name == "Normals") {
                    data.normals = createVec3Vector(getDoubleVector(subdata));

                } else if (subdata.name == "NormalsIndex") {
                    data.normalIndices = getIntVector(subdata);

                } else if (subdata.name == "MappingInformationType" && subdata.properties.at(0) == "ByVertice") {
                    data.normalsByVertex = true;
                    
                } else if (subdata.name == "ReferenceInformationType" && subdata.properties.at(0) == "IndexToDirect") {
                    indexToDirect = true;
                }
            }
            if (indexToDirect && data.normalIndices.isEmpty()) {
                // hack to work around wacky Makehuman exports
                data.normalsByVertex = true;
            }
        } else if (child.name == "LayerElementUV" && child.properties.at(0).toInt() == 0) {
            foreach (const FBXNode& subdata, child.children) {
                if (subdata.name == "UV") {
                    data.texCoords = createVec2Vector(getDoubleVector(subdata));

                } else if (subdata.name == "UVIndex") {
                    data.texCoordIndices = getIntVector(subdata);
                }
            }
        } else if (child.name == "LayerElementMaterial") {
            foreach (const FBXNode& subdata, child.children) {
                if (subdata.name == "Materials") {
                    materials = getIntVector(subdata);
                }
            }
        } else if (child.name == "LayerElementTexture") {
            foreach (const FBXNode& subdata, child.children) {
                if (subdata.name == "TextureId") {
                    textures = getIntVector(subdata);
                }
            }
        }
    }

    // convert the polygons to quads and triangles
    int polygonIndex = 0;
    QHash<QPair<int, int>, int> materialTextureParts;
    for (int beginIndex = 0; beginIndex < data.polygonIndices.size(); polygonIndex++) {
        int endIndex = beginIndex;
        while (endIndex < data.polygonIndices.size() && data.polygonIndices.at(endIndex++) >= 0);

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
                if (nextIndex >= data.polygonIndices.size() || data.polygonIndices.at(nextIndex) < 0) {
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
            blendshape.indices = getIntVector(data);

        } else if (data.name == "Vertices") {
            blendshape.vertices = createVec3Vector(getDoubleVector(data));

        } else if (data.name == "Normals") {
            blendshape.normals = createVec3Vector(getDoubleVector(data));
        }
    }
    return blendshape;
}

void setTangents(FBXMesh& mesh, int firstIndex, int secondIndex) {
    const glm::vec3& normal = mesh.normals.at(firstIndex);
    glm::vec3 bitangent = glm::cross(normal, mesh.vertices.at(secondIndex) - mesh.vertices.at(firstIndex));
    if (glm::length(bitangent) < EPSILON) {
        return;
    }
    glm::vec2 texCoordDelta = mesh.texCoords.at(secondIndex) - mesh.texCoords.at(firstIndex);
    glm::vec3 normalizedNormal = glm::normalize(normal);
    mesh.tangents[firstIndex] += glm::cross(glm::angleAxis(-atan2f(-texCoordDelta.t, texCoordDelta.s), normalizedNormal) *
        glm::normalize(bitangent), normalizedNormal);
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
    JointShapeInfo() : numVertices(0), 
            sumVertexWeights(0.0f), sumWeightedRadii(0.0f), numVertexWeights(0), 
            averageVertex(0.f), boneBegin(0.f), averageRadius(0.f) {
    }

    // NOTE: the points here are in the "joint frame" which has the "jointEnd" at the origin
    int numVertices;        // num vertices from contributing meshes
    float sumVertexWeights; // sum of all vertex weights
    float sumWeightedRadii; // sum of weighted vertices
    int numVertexWeights;   // num vertices that contributed to sums
    glm::vec3 averageVertex;// average of all mesh vertices (in joint frame)
    glm::vec3 boneBegin;    // parent joint location (in joint frame)
    float averageRadius;    // average distance from mesh points to averageVertex
};

class AnimationCurve {
public:
    QVector<float> values;
};

FBXTexture getTexture(const QString& textureID, const QHash<QString, QByteArray>& textureFilenames,
        const QHash<QByteArray, QByteArray>& textureContent) {
    FBXTexture texture;
    texture.filename = textureFilenames.value(textureID);
    texture.content = textureContent.value(texture.filename);
    return texture;
}

bool checkMaterialsHaveTextures(const QHash<QString, Material>& materials,
        const QHash<QString, QByteArray>& textureFilenames, const QMultiHash<QString, QString>& childMap) {
    foreach (const QString& materialID, materials.keys()) {
        foreach (const QString& childID, childMap.values(materialID)) {
            if (textureFilenames.contains(childID)) {
                return true;
            }
        }
    }
    return false;
}

FBXGeometry extractFBXGeometry(const FBXNode& node, const QVariantHash& mapping) {
    QHash<QString, ExtractedMesh> meshes;
    QVector<ExtractedBlendshape> blendshapes;
    QMultiHash<QString, QString> parentMap;
    QMultiHash<QString, QString> childMap;
    QHash<QString, FBXModel> models;
    QHash<QString, Cluster> clusters;
    QHash<QString, AnimationCurve> animationCurves;
    QHash<QString, QByteArray> textureFilenames;
    QHash<QByteArray, QByteArray> textureContent;
    QHash<QString, Material> materials;
    QHash<QString, QString> diffuseTextures;
    QHash<QString, QString> bumpTextures;
    QHash<QString, QString> specularTextures;
    QHash<QString, QString> localRotations;
    QHash<QString, QString> xComponents;
    QHash<QString, QString> yComponents;
    QHash<QString, QString> zComponents;

    QVariantHash joints = mapping.value("joint").toHash();
    QString jointEyeLeftName = processID(getString(joints.value("jointEyeLeft", "jointEyeLeft")));
    QString jointEyeRightName = processID(getString(joints.value("jointEyeRight", "jointEyeRight")));
    QString jointNeckName = processID(getString(joints.value("jointNeck", "jointNeck")));
    QString jointRootName = processID(getString(joints.value("jointRoot", "jointRoot")));
    QString jointLeanName = processID(getString(joints.value("jointLean", "jointLean")));
    QString jointHeadName = processID(getString(joints.value("jointHead", "jointHead")));
    QString jointLeftHandName = processID(getString(joints.value("jointLeftHand", "jointLeftHand")));
    QString jointRightHandName = processID(getString(joints.value("jointRightHand", "jointRightHand")));
    QString jointEyeLeftID;
    QString jointEyeRightID;
    QString jointNeckID;
    QString jointRootID;
    QString jointLeanID;
    QString jointHeadID;
    QString jointLeftHandID;
    QString jointRightHandID;
    
    QVector<QString> humanIKJointNames;
    for (int i = 0;; i++) {
        QByteArray jointName = HUMANIK_JOINTS[i];
        if (jointName.isEmpty()) {
            break;
        }
        humanIKJointNames.append(processID(getString(joints.value(jointName, jointName))));
    }
    QVector<QString> humanIKJointIDs(humanIKJointNames.size());

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
    
    FBXGeometry geometry;
    foreach (const FBXNode& child, node.children) {
        if (child.name == "FBXHeaderExtension") {
            foreach (const FBXNode& object, child.children) {
                if (object.name == "SceneInfo") {
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "MetaData") {
                            foreach (const FBXNode& subsubobject, subobject.children) {
                                if (subsubobject.name == "Author") {
                                    geometry.author = subsubobject.properties.at(0).toString();
                                }
                            } 
                        } else if (subobject.name == "Properties70") {
                            foreach (const FBXNode& subsubobject, subobject.children) {
                                if (subsubobject.name == "P" && subsubobject.properties.size() >= 5 &&
                                        subsubobject.properties.at(0) == "Original|ApplicationName") {
                                    geometry.applicationName = subsubobject.properties.at(4).toString();
                                }
                            }
                        }
                    }
                }
            }
        } else if (child.name == "Objects") {
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
                        name = processID(name.left(name.indexOf(QChar('\0'))));

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
                    int humanIKJointIndex = humanIKJointNames.indexOf(name);
                    if (humanIKJointIndex != -1) {
                        humanIKJointIDs[humanIKJointIndex] = getID(object.properties);
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
                    // NOTE: angles from the FBX file are in degrees
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
                } else if (object.name == "Video") {
                    QByteArray filename;
                    QByteArray content;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "RelativeFilename") {
                            filename = subobject.properties.at(0).toByteArray();
                            filename = filename.mid(qMax(filename.lastIndexOf('\\'), filename.lastIndexOf('/')) + 1);
                            
                        } else if (subobject.name == "Content" && !subobject.properties.isEmpty()) {
                            content = subobject.properties.at(0).toByteArray();
                        }
                    }
                    if (!content.isEmpty()) {
                        textureContent.insert(filename, content);
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
                                cluster.indices = getIntVector(subobject);

                            } else if (subobject.name == "Weights") {
                                cluster.weights = getDoubleVector(subobject);

                            } else if (subobject.name == "TransformLink") {
                                QVector<double> values = getDoubleVector(subobject);
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
                } else if (object.name == "AnimationCurve") {
                    AnimationCurve curve;
                    foreach (const FBXNode& subobject, object.children) {
                        if (subobject.name == "KeyValueFloat") {
                            curve.values = getFloatVector(subobject);
                        }
                    }
                    animationCurves.insert(getID(object.properties), curve);
                }
            }
        } else if (child.name == "Connections") {
            foreach (const FBXNode& connection, child.children) {
                if (connection.name == "C" || connection.name == "Connect") {
                    if (connection.properties.at(0) == "OP") {
                        QByteArray type = connection.properties.at(3).toByteArray().toLower();
                        if (type.contains("diffuse")) {
                            diffuseTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));

                        } else if (type.contains("bump") || type.contains("normal")) {
                            bumpTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        
                        } else if (type.contains("specular") || type.contains("reflection")) {
                            specularTextures.insert(getID(connection.properties, 2), getID(connection.properties, 1));    
                            
                        } else if (type == "lcl rotation") {
                            localRotations.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                            
                        } else if (type == "d|x") {
                            xComponents.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                        
                        } else if (type == "d|y") {
                            yComponents.insert(getID(connection.properties, 2), getID(connection.properties, 1));
                            
                        } else if (type == "d|z") {
                            zComponents.insert(getID(connection.properties, 2), getID(connection.properties, 1));
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
    float offsetScale = mapping.value("scale", 1.0f).toFloat();
    glm::quat offsetRotation = glm::quat(glm::radians(glm::vec3(mapping.value("rx").toFloat(),
            mapping.value("ry").toFloat(), mapping.value("rz").toFloat())));
    geometry.offset = glm::translate(glm::vec3(mapping.value("tx").toFloat(), mapping.value("ty").toFloat(),
        mapping.value("tz").toFloat())) * glm::mat4_cast(offsetRotation) *
            glm::scale(glm::vec3(offsetScale, offsetScale, offsetScale));

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
        QString first = *remainingModels.constBegin();
        foreach (const QString& id, remainingModels) {
            if (id < first) {
                first = id;
            }
        }
        QString topID = getTopModelID(parentMap, models, first);
        appendModelIDs(parentMap.value(topID), childMap, models, remainingModels, modelIDs);
    }

    // figure the number of animation frames from the curves
    int frameCount = 1;
    foreach (const AnimationCurve& curve, animationCurves) {
        frameCount = qMax(frameCount, curve.values.size());
    }
    for (int i = 0; i < frameCount; i++) {
        FBXAnimationFrame frame;
        frame.rotations.resize(modelIDs.size());
        geometry.animationFrames.append(frame);
    }
    
    // convert the models to joints
    QVariantList freeJoints = mapping.values("freeJoint");
    foreach (const QString& modelID, modelIDs) {
        const FBXModel& model = models[modelID];
        FBXJoint joint;
        joint.isFree = freeJoints.contains(model.name);
        joint.parentIndex = model.parentIndex;

        // get the indices of all ancestors starting with the first free one (if any)
        int jointIndex = geometry.joints.size();
        joint.freeLineage.append(jointIndex);
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
        
        QString rotationID = localRotations.value(modelID);
        AnimationCurve xCurve = animationCurves.value(xComponents.value(rotationID));
        AnimationCurve yCurve = animationCurves.value(yComponents.value(rotationID));
        AnimationCurve zCurve = animationCurves.value(zComponents.value(rotationID));
        glm::vec3 defaultValues = glm::degrees(safeEulerAngles(joint.rotation));
        for (int i = 0; i < frameCount; i++) {
            geometry.animationFrames[i].rotations[jointIndex] = glm::quat(glm::radians(glm::vec3(
                xCurve.values.isEmpty() ? defaultValues.x : xCurve.values.at(i % xCurve.values.size()),
                yCurve.values.isEmpty() ? defaultValues.y : yCurve.values.at(i % yCurve.values.size()),
                zCurve.values.isEmpty() ? defaultValues.z : zCurve.values.at(i % zCurve.values.size()))));
        }
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
    
    foreach (const QString& id, humanIKJointIDs) {
        geometry.humanIKJointIndices.append(modelIDs.indexOf(id));
    }
    
    // extract the translation component of the neck transform
    if (geometry.neckJointIndex != -1) {
        const glm::mat4& transform = geometry.joints.at(geometry.neckJointIndex).transform;
        geometry.neckPivot = glm::vec3(transform[3][0], transform[3][1], transform[3][2]);
    }

    geometry.bindExtents.reset();
    geometry.meshExtents.reset();
    
    // see if any materials have texture children
    bool materialsHaveTextures = checkMaterialsHaveTextures(materials, textureFilenames, childMap);
    
    for (QHash<QString, ExtractedMesh>::iterator it = meshes.begin(); it != meshes.end(); it++) {
        ExtractedMesh& extracted = it.value();
        
        extracted.mesh.meshExtents.reset();

        // accumulate local transforms
        QString modelID = models.contains(it.key()) ? it.key() : parentMap.value(it.key());
        glm::mat4 modelTransform = getGlobalTransform(parentMap, models, modelID, geometry.applicationName == "mixamo.com");

        // compute the mesh extents from the transformed vertices
        foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
            glm::vec3 transformedVertex = glm::vec3(modelTransform * glm::vec4(vertex, 1.0f));
            geometry.meshExtents.minimum = glm::min(geometry.meshExtents.minimum, transformedVertex);
            geometry.meshExtents.maximum = glm::max(geometry.meshExtents.maximum, transformedVertex);

            extracted.mesh.meshExtents.minimum = glm::min(extracted.mesh.meshExtents.minimum, transformedVertex);
            extracted.mesh.meshExtents.maximum = glm::max(extracted.mesh.meshExtents.maximum, transformedVertex);
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
                
                FBXTexture diffuseTexture;
                QString diffuseTextureID = diffuseTextures.value(childID);
                if (!diffuseTextureID.isNull()) {
                    diffuseTexture = getTexture(diffuseTextureID, textureFilenames, textureContent);
                    
                    // FBX files generated by 3DSMax have an intermediate texture parent, apparently
                    foreach (const QString& childTextureID, childMap.values(diffuseTextureID)) {
                        if (textureFilenames.contains(childTextureID)) {
                            diffuseTexture = getTexture(diffuseTextureID, textureFilenames, textureContent);
                        }
                    }
                }
                
                FBXTexture normalTexture;
                QString bumpTextureID = bumpTextures.value(childID);
                if (!bumpTextureID.isNull()) {
                    normalTexture = getTexture(bumpTextureID, textureFilenames, textureContent);
                    generateTangents = true;
                }
                
                FBXTexture specularTexture;
                QString specularTextureID = specularTextures.value(childID);
                if (!specularTextureID.isNull()) {
                    specularTexture = getTexture(specularTextureID, textureFilenames, textureContent);
                }
                
                for (int j = 0; j < extracted.partMaterialTextures.size(); j++) {
                    if (extracted.partMaterialTextures.at(j).first == materialIndex) {
                        FBXMeshPart& part = extracted.mesh.parts[j];
                        part.diffuseColor = material.diffuse;
                        part.specularColor = material.specular;
                        part.shininess = material.shininess;
                        if (!diffuseTexture.filename.isNull()) {
                            part.diffuseTexture = diffuseTexture;
                        }
                        if (!normalTexture.filename.isNull()) {
                            part.normalTexture = normalTexture;
                        }
                        if (!specularTexture.filename.isNull()) {
                            part.specularTexture = specularTexture;
                        }
                    }
                }
                materialIndex++;
                
            } else if (textureFilenames.contains(childID)) {
                FBXTexture texture = getTexture(childID, textureFilenames, textureContent);
                for (int j = 0; j < extracted.partMaterialTextures.size(); j++) {
                    int partTexture = extracted.partMaterialTextures.at(j).second;
                    if (partTexture == textureIndex && !(partTexture == 0 && materialsHaveTextures)) {
                        extracted.mesh.parts[j].diffuseTexture = texture;
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
                // <= size - 3 in order to prevent overflowing triangleIndices when (i % 3) != 0 
                // This is most likely evidence of a further problem in extractMesh()
                for (int i = 0; i <= part.triangleIndices.size() - 3; i += 3) {
                    setTangents(extracted.mesh, part.triangleIndices.at(i), part.triangleIndices.at(i + 1));
                    setTangents(extracted.mesh, part.triangleIndices.at(i + 1), part.triangleIndices.at(i + 2));
                    setTangents(extracted.mesh, part.triangleIndices.at(i + 2), part.triangleIndices.at(i));
                }
                if ((part.triangleIndices.size() % 3) != 0){
                    qDebug() << "Error in extractFBXGeometry part.triangleIndices.size() is not divisible by three ";
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
                float boneLength = 0.0f;
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
                            float proj = glm::dot(boneDirection, boneEnd - vertex);
                            float radiusWeight = (proj < 0.0f || proj > boneLength) ? 0.5f * weight : weight;

                            jointShapeInfo.sumVertexWeights += radiusWeight;
                            jointShapeInfo.sumWeightedRadii += radiusWeight * radiusScale * glm::distance(vertex, boneEnd - boneDirection * proj);
                            ++jointShapeInfo.numVertexWeights;

                            glm::vec3 vertexInJointFrame = rotateMeshToJoint * (radiusScale * (vertex - boneEnd));
                            jointShapeInfo.averageVertex += vertexInJointFrame;
                            ++jointShapeInfo.numVertices;
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
            float boneLength = 0.0f;
            if (joint.parentIndex != -1) {
                boneBegin = extractTranslation(inverseModelTransform * geometry.joints[joint.parentIndex].bindTransform);
                boneDirection = boneEnd - boneBegin;
                boneLength = glm::length(boneDirection);
                if (boneLength > EPSILON) {
                    boneDirection /= boneLength;
                }
            }
            float radiusScale = extractUniformScale(joint.transform * firstFBXCluster.inverseBindMatrix);

            glm::vec3 averageVertex(0.f);
            foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
                float proj = glm::dot(boneDirection, boneEnd - vertex);
                float radiusWeight = (proj < 0.0f || proj > boneLength) ? 0.5f : 1.0f;
                jointShapeInfo.sumVertexWeights += radiusWeight;
                jointShapeInfo.sumWeightedRadii += radiusWeight * radiusScale * glm::distance(vertex, boneEnd - boneDirection * proj);
                ++jointShapeInfo.numVertexWeights;

                glm::vec3 vertexInJointFrame = rotateMeshToJoint * (radiusScale * (vertex - boneEnd));
                jointShapeInfo.averageVertex += vertexInJointFrame;
                averageVertex += vertex;
            }
            int numVertices = extracted.mesh.vertices.size();
            jointShapeInfo.numVertices = numVertices;
            if (numVertices > 0) {
                averageVertex /= (float)jointShapeInfo.numVertices;
                float averageRadius = 0.f;
                foreach (const glm::vec3& vertex, extracted.mesh.vertices) {
                    averageRadius += glm::distance(vertex, averageVertex);
                }
                jointShapeInfo.averageRadius = averageRadius * radiusScale;
            }
        }
        extracted.mesh.isEye = (maxJointIndex == geometry.leftEyeJointIndex || maxJointIndex == geometry.rightEyeJointIndex);
        
        geometry.meshes.append(extracted.mesh);
    }

    // now that all joints have been scanned, compute a collision shape for each joint
    glm::vec3 defaultCapsuleAxis(0.f, 1.f, 0.f);
    for (int i = 0; i < geometry.joints.size(); ++i) {
        FBXJoint& joint = geometry.joints[i];
        JointShapeInfo& jointShapeInfo = jointShapeInfos[i];

        if (joint.parentIndex == -1) {
            jointShapeInfo.boneBegin = glm::vec3(0.0f);
        } else {
            const FBXJoint& parentJoint = geometry.joints[joint.parentIndex];
            glm::quat inverseRotation = glm::inverse(extractRotation(joint.transform));
            jointShapeInfo.boneBegin = inverseRotation * (extractTranslation(parentJoint.transform) - extractTranslation(joint.transform));
        }

        if (jointShapeInfo.sumVertexWeights > 0.0f) {
            joint.boneRadius = jointShapeInfo.sumWeightedRadii / jointShapeInfo.sumVertexWeights;
        }

        // we use a capsule if the joint had ANY mesh vertices successfully projected onto the bone
        // AND its boneRadius is not too close to zero
        bool collideLikeCapsule = jointShapeInfo.numVertexWeights > 0
                && glm::length(jointShapeInfo.boneBegin) > EPSILON;

        if (collideLikeCapsule) {
            joint.shapeRotation = rotationBetween(defaultCapsuleAxis, jointShapeInfo.boneBegin);
            joint.shapePosition = 0.5f * jointShapeInfo.boneBegin;
            joint.shapeType = Shape::CAPSULE_SHAPE;
        } else {
            // collide the joint like a sphere
            joint.shapeType = Shape::SPHERE_SHAPE;
            if (jointShapeInfo.numVertices > 0) {
                jointShapeInfo.averageVertex /= (float)jointShapeInfo.numVertices;
                joint.shapePosition = jointShapeInfo.averageVertex;
            } else {
                joint.shapePosition = glm::vec3(0.f);
            }
            if (jointShapeInfo.numVertexWeights == 0
                   && jointShapeInfo.numVertices > 0) {
                // the bone projection algorithm was not able to compute the joint radius
                // so we use an alternative measure
                jointShapeInfo.averageRadius /= (float)jointShapeInfo.numVertices;
                joint.boneRadius = jointShapeInfo.averageRadius;
            }

            float distanceFromEnd = glm::length(joint.shapePosition);
            float distanceFromBegin = glm::distance(joint.shapePosition, jointShapeInfo.boneBegin);
            if (distanceFromEnd > joint.distanceToParent && distanceFromBegin > joint.distanceToParent) {
                // The shape is further from both joint endpoints than the endpoints are from each other
                // which probably means the model has a bad transform somewhere.  We disable this shape
                // by setting its type to UNKNOWN_SHAPE.
                joint.shapeType = Shape::UNKNOWN_SHAPE;
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
    
    // Add sitting points
    QVariantHash sittingPoints = mapping.value("sit").toHash();
    for (QVariantHash::const_iterator it = sittingPoints.constBegin(); it != sittingPoints.constEnd(); it++) {
        SittingPoint sittingPoint;
        sittingPoint.name = it.key();
        
        QVariantList properties = it->toList();
        sittingPoint.position = parseVec3(properties.at(0).toString());
        sittingPoint.rotation = glm::quat(glm::radians(parseVec3(properties.at(1).toString())));
        
        geometry.sittingPoints.append(sittingPoint);
    }
    
    return geometry;
}

QVariantHash readMapping(const QByteArray& data) {
    QBuffer buffer(const_cast<QByteArray*>(&data));
    buffer.open(QIODevice::ReadOnly);
    return parseMapping(&buffer);
}

QByteArray writeMapping(const QVariantHash& mapping) {
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    for (QVariantHash::const_iterator first = mapping.constBegin(); first != mapping.constEnd(); first++) {
        QByteArray key = first.key().toUtf8() + " = ";
        QVariantHash hashValue = first.value().toHash();
        if (hashValue.isEmpty()) {
            buffer.write(key + first.value().toByteArray() + "\n");
            continue;
        }
        for (QVariantHash::const_iterator second = hashValue.constBegin(); second != hashValue.constEnd(); second++) {
            QByteArray extendedKey = key + second.key().toUtf8();
            QVariantList listValue = second.value().toList();
            if (listValue.isEmpty()) {
                buffer.write(extendedKey + " = " + second.value().toByteArray() + "\n");
                continue;
            }
            buffer.write(extendedKey);
            for (QVariantList::const_iterator third = listValue.constBegin(); third != listValue.constEnd(); third++) {
                buffer.write(" = " + third->toByteArray());
            }
            buffer.write("\n");
        }
    }
    return buffer.data();
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

void calculateRotatedExtents(Extents& extents, const glm::quat& rotation) {
    glm::vec3 bottomLeftNear(extents.minimum.x, extents.minimum.y, extents.minimum.z);
    glm::vec3 bottomRightNear(extents.maximum.x, extents.minimum.y, extents.minimum.z);
    glm::vec3 bottomLeftFar(extents.minimum.x, extents.minimum.y, extents.maximum.z);
    glm::vec3 bottomRightFar(extents.maximum.x, extents.minimum.y, extents.maximum.z);
    glm::vec3 topLeftNear(extents.minimum.x, extents.maximum.y, extents.minimum.z);
    glm::vec3 topRightNear(extents.maximum.x, extents.maximum.y, extents.minimum.z);
    glm::vec3 topLeftFar(extents.minimum.x, extents.maximum.y, extents.maximum.z);
    glm::vec3 topRightFar(extents.maximum.x, extents.maximum.y, extents.maximum.z);

    glm::vec3 bottomLeftNearRotated =  rotation * bottomLeftNear;
    glm::vec3 bottomRightNearRotated = rotation * bottomRightNear;
    glm::vec3 bottomLeftFarRotated = rotation * bottomLeftFar;
    glm::vec3 bottomRightFarRotated = rotation * bottomRightFar;
    glm::vec3 topLeftNearRotated = rotation * topLeftNear;
    glm::vec3 topRightNearRotated = rotation * topRightNear;
    glm::vec3 topLeftFarRotated = rotation * topLeftFar;
    glm::vec3 topRightFarRotated = rotation * topRightFar;
    
    extents.minimum = glm::min(bottomLeftNearRotated,
                        glm::min(bottomRightNearRotated,
                        glm::min(bottomLeftFarRotated,
                        glm::min(bottomRightFarRotated,
                        glm::min(topLeftNearRotated,
                        glm::min(topRightNearRotated,
                        glm::min(topLeftFarRotated,topRightFarRotated)))))));

    extents.maximum = glm::max(bottomLeftNearRotated,
                        glm::max(bottomRightNearRotated,
                        glm::max(bottomLeftFarRotated,
                        glm::max(bottomRightFarRotated,
                        glm::max(topLeftNearRotated,
                        glm::max(topRightNearRotated,
                        glm::max(topLeftFarRotated,topRightFarRotated)))))));
}

