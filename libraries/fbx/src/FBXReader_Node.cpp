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
#include <QFileInfo>
#include "FBXReader.h"

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

FBXNode parseBinaryFBXNode(QDataStream& in, int& position) {
    qint32 endOffset;
    quint32 propertyCount;
    quint32 propertyListLength;
    quint8 nameLength;

    in >> endOffset;
    in >> propertyCount;
    in >> propertyListLength;
    in >> nameLength;
    position += sizeof(quint32) * 3 + sizeof(quint8);

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
        FBXNode child = parseBinaryFBXNode(in, position);
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
    int position = HEADER_SIZE;

    // parse the top-level node
    FBXNode top;
    while (device->bytesAvailable()) {
        FBXNode next = parseBinaryFBXNode(in, position);
        if (next.name.isNull()) {
            return top;

        } else {
            top.children.append(next);
        }
    }

    return top;
}

