//
//  FBXReader_Node.cpp
//  interface/src/fbx
//
//  Created by Sam Gateau on 8/27/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "FBXReader.h"

#include <iostream>
#include <QtCore/QBuffer>
#include <QtCore/QDataStream>
#include <QtCore/QIODevice>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <QtCore/QtEndian>
#include <QtCore/QFileInfo>

#include <shared/NsightHelpers.h>
#include "ModelFormatLogging.h"

template<class T> int streamSize() {
    return sizeof(T);
}

template<bool> int streamSize() {
    return 1;
}

template<class T> QVariant readBinaryArray(QDataStream& in, int& position) {
    quint32 arrayLength;
    quint32 encoding;
    quint32 compressedLength;

    in >> arrayLength;
    in >> encoding;
    in >> compressedLength;
    position += sizeof(quint32) * 3;

    QVector<T> values;
    const unsigned int DEFLATE_ENCODING = 1;
    if (encoding == DEFLATE_ENCODING) {
        // preface encoded data with uncompressed length
        QByteArray compressed(sizeof(quint32) + compressedLength, 0);
        *((quint32*)compressed.data()) = qToBigEndian<quint32>(arrayLength * sizeof(T));
        in.readRawData(compressed.data() + sizeof(quint32), compressedLength);
        position += compressedLength;
        QByteArray uncompressed = qUncompress(compressed);
        if (uncompressed.isEmpty()) { // answers empty byte array if corrupt
            throw QString("corrupt fbx file");
        }
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
            position += streamSize<T>();
            values.append(value);
        }
    }
    return QVariant::fromValue(values);
}

QVariant parseBinaryFBXProperty(QDataStream& in, int& position) {
    char ch;
    in.device()->getChar(&ch);
    position++;
    switch (ch) {
        case 'Y': {
            qint16 value;
            in >> value;
            position += sizeof(qint16);
            return QVariant::fromValue(value);
        }
        case 'C': {
            bool value;
            in >> value;
            position++;
            return QVariant::fromValue(value);
        }
        case 'I': {
            qint32 value;
            in >> value;
            position += sizeof(qint32);
            return QVariant::fromValue(value);
        }
        case 'F': {
            float value;
            in >> value;
            position += sizeof(float);
            return QVariant::fromValue(value);
        }
        case 'D': {
            double value;
            in >> value;
            position += sizeof(double);
            return QVariant::fromValue(value);
        }
        case 'L': {
            qint64 value;
            in >> value;
            position += sizeof(qint64);
            return QVariant::fromValue(value);
        }
        case 'f': {
            return readBinaryArray<float>(in, position);
        }
        case 'd': {
            return readBinaryArray<double>(in, position);
        }
        case 'l': {
            return readBinaryArray<qint64>(in, position);
        }
        case 'i': {
            return readBinaryArray<qint32>(in, position);
        }
        case 'b': {
            return readBinaryArray<bool>(in, position);
        }
        case 'S':
        case 'R': {
            quint32 length;
            in >> length;
            position += sizeof(quint32) + length;
            return QVariant::fromValue(in.device()->read(length));
        }
        default:
            throw QString("Unknown property type: ") + ch;
    }
}

FBXNode parseBinaryFBXNode(QDataStream& in, int& position, bool has64BitPositions = false) {
    qint64 endOffset;
    quint64 propertyCount;
    quint64 propertyListLength;
    quint8 nameLength;

    // FBX 2016 and beyond uses 64bit positions in the node headers, pre-2016 used 32bit values
    // our code generally doesn't care about the size that much, so we will use 64bit values
    // from here on out, but if the file is an older format we read the stream into temp 32bit 
    // values and then assign to our actual 64bit values.
    if (has64BitPositions) {
        in >> endOffset;
        in >> propertyCount;
        in >> propertyListLength;
        position += sizeof(quint64) * 3;
    } else {
        qint32 tempEndOffset;
        quint32 tempPropertyCount;
        quint32 tempPropertyListLength;
        in >> tempEndOffset;
        in >> tempPropertyCount;
        in >> tempPropertyListLength;
        position += sizeof(quint32) * 3;
        endOffset = tempEndOffset;
        propertyCount = tempPropertyCount;
        propertyListLength = tempPropertyListLength;
    }
    in >> nameLength;
    position += sizeof(quint8);

    FBXNode node;
    const int MIN_VALID_OFFSET = 40;
    if (endOffset < MIN_VALID_OFFSET || nameLength == 0) {
        // use a null name to indicate a null node
        return node;
    }
    node.name = in.device()->read(nameLength);
    position += nameLength;

    for (quint32 i = 0; i < propertyCount; i++) {
        node.properties.append(parseBinaryFBXProperty(in, position));
    }

    while (endOffset > position) {
        FBXNode child = parseBinaryFBXNode(in, position, has64BitPositions);
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

    enum SpecialToken {
        NO_TOKEN = -1,
        NO_PUSHBACKED_TOKEN = -1,
        DATUM_TOKEN = 0x100
    };

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
    return NO_TOKEN;
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
    while ((token = tokenizer.nextToken()) != Tokenizer::NO_TOKEN) {
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

FBXNode FBXReader::parseFBX(QIODevice* device) {
    PROFILE_RANGE_EX(modelformat, __FUNCTION__, 0xff0000ff, device);
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

    // The first 27 bytes contain the header.
    //   Bytes 0 - 20: Kaydara FBX Binary  \x00(file - magic, with 2 spaces at the end, then a NULL terminator).
    //   Bytes 21 - 22: [0x1A, 0x00](unknown but all observed files show these bytes).
    //   Bytes 23 - 26 : unsigned int, the version number. 7300 for version 7.3 for example.
    const int HEADER_BEFORE_VERSION = 23;
    const quint32 VERSION_FBX2016 = 7500;
    in.skipRawData(HEADER_BEFORE_VERSION);
    int position = HEADER_BEFORE_VERSION;
    quint32 fileVersion;
    in >> fileVersion;
    position += sizeof(fileVersion);
    qCDebug(modelformat) << "fileVersion:" << fileVersion;
    bool has64BitPositions = (fileVersion >= VERSION_FBX2016);

    // parse the top-level node
    FBXNode top;
    while (device->bytesAvailable()) {
        FBXNode next = parseBinaryFBXNode(in, position, has64BitPositions);
        if (next.name.isNull()) {
            return top;

        } else {
            top.children.append(next);
        }
    }

    return top;
}


glm::vec3 FBXReader::getVec3(const QVariantList& properties, int index) {
    return glm::vec3(properties.at(index).value<double>(), properties.at(index + 1).value<double>(),
        properties.at(index + 2).value<double>());
}

QVector<glm::vec4> FBXReader::createVec4Vector(const QVector<double>& doubleVector) {
    QVector<glm::vec4> values;
    for (const double* it = doubleVector.constData(), *end = it + ((doubleVector.size() / 4) * 4); it != end; ) {
        float x = *it++;
        float y = *it++;
        float z = *it++;
        float w = *it++;
        values.append(glm::vec4(x, y, z, w));
    }
    return values;
}


QVector<glm::vec4> FBXReader::createVec4VectorRGBA(const QVector<double>& doubleVector, glm::vec4& average) {
    QVector<glm::vec4> values;
    for (const double* it = doubleVector.constData(), *end = it + ((doubleVector.size() / 4) * 4); it != end; ) {
        float x = *it++;
        float y = *it++;
        float z = *it++;
        float w = *it++;
        auto val = glm::vec4(x, y, z, w);
        values.append(val);
        average += val;
    }
    if (!values.isEmpty()) {
        average *= (1.0f / float(values.size()));
    }
    return values;
}

QVector<glm::vec3> FBXReader::createVec3Vector(const QVector<double>& doubleVector) {
    QVector<glm::vec3> values;
    for (const double* it = doubleVector.constData(), *end = it + ((doubleVector.size() / 3) * 3); it != end; ) {
        float x = *it++;
        float y = *it++;
        float z = *it++;
        values.append(glm::vec3(x, y, z));
    }
    return values;
}

QVector<glm::vec2> FBXReader::createVec2Vector(const QVector<double>& doubleVector) {
    QVector<glm::vec2> values;
    for (const double* it = doubleVector.constData(), *end = it + ((doubleVector.size() / 2) * 2); it != end; ) {
        float s = *it++;
        float t = *it++;
        values.append(glm::vec2(s, -t));
    }
    return values;
}

glm::mat4 FBXReader::createMat4(const QVector<double>& doubleVector) {
    return glm::mat4(doubleVector.at(0), doubleVector.at(1), doubleVector.at(2), doubleVector.at(3),
        doubleVector.at(4), doubleVector.at(5), doubleVector.at(6), doubleVector.at(7),
        doubleVector.at(8), doubleVector.at(9), doubleVector.at(10), doubleVector.at(11),
        doubleVector.at(12), doubleVector.at(13), doubleVector.at(14), doubleVector.at(15));
}

QVector<int> FBXReader::getIntVector(const FBXNode& node) {
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

QVector<float> FBXReader::getFloatVector(const FBXNode& node) {
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

QVector<double> FBXReader::getDoubleVector(const FBXNode& node) {
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

