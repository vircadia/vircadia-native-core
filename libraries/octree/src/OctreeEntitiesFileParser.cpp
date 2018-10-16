//
//  OctreeEntititesFileParser.cpp
//  libraries/octree/src
//
//  Created by Simon Walton on Oct 15, 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <sstream>
#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantList>

#include "OctreeEntitiesFileParser.h"

using std::string;

OctreeEntitiesFileParser::OctreeEntitiesFileParser() {
}

std::string OctreeEntitiesFileParser::getErrorString() const {
    std::ostringstream err;
    if (_errorString.size() != 0) {
        err << "Error: Line " << _line << ", byte position " << _position << ": " << _errorString;
    };

    return err.str();
}

void OctreeEntitiesFileParser::setEntitiesString(const QByteArray& entitiesContents) {
    _entitiesContents = entitiesContents;
    _entitiesLength = _entitiesContents.length();
    _position = 0;
    _line = 1;
}

bool OctreeEntitiesFileParser::parseEntities(QVariantMap& parsedEntities) {
    if (nextToken() != '{') {
        _errorString = "Text before start of object";
        return false;
    }

    bool gotDataVersion = false;
    bool gotEntities = false;
    bool gotId = false;
    bool gotVersion = false;

    while (!(gotDataVersion && gotEntities && gotId && gotVersion)) {
        if (nextToken() != '"') {
            _errorString = "Incorrect key string";
            return false;
        }

        string key = readString();
        if (key.size() == 0) {
            _errorString = "Missing object key";
            return false;
        }

        if (nextToken() != ':') {
            _errorString = "Ill-formed id/value entry";
            return false;
        }

        if (key == "DataVersion") {
            if (gotDataVersion) {
                _errorString = "Duplicate DataVersion entries";
                return false;
            }

            int dataVersionValue = readInteger();
            parsedEntities["DataVersion"] = dataVersionValue;
            gotDataVersion = true;
        } else if (key == "Entities") {
            if (gotEntities) {
                _errorString = "Duplicate Entities entries";
                return false;
            }

            QVariantList entitiesValue;
            if (!readEntitiesArray(entitiesValue)) {
                return false;
            }

            parsedEntities["Entities"] = entitiesValue;
            gotEntities = true;
        } else if (key == "Id") {
            if (gotId) {
                _errorString = "Duplicate Id entries";
                return false;
            }

            if (nextToken() != '"') {
                _errorString = "Invalid Id value";
                return false;
            };
            string idString = readString();
            if (idString.size() == 0) {
                _errorString = "Invalid Id string";
                return false;
            }
            QUuid idValue = QUuid::fromString(QLatin1String(idString.c_str()) );
            if (idValue.isNull()) {
                _errorString = "Id value invalid UUID string: " + idString;
                return false;
            }

            parsedEntities["Id"] = idValue;
            gotId = true;
        } else if (key == "Version") {
            if (gotVersion) {
                _errorString = "Duplicate Version entries";
                return false;
            }

            int versionValue = readInteger();
            parsedEntities["Version"] = versionValue;
            gotVersion = true;
        } else {
            _errorString = "Unrecognized key name: " + key;
            return false;
        }

        if (gotDataVersion && gotEntities && gotId && gotVersion) {
            break;
        } else if (nextToken() != ',') {
            _errorString = "Id/value incorrectly terminated";
            return false;
        }
    }

    if (nextToken() != '}' || nextToken() != -1) {
        _errorString = "Ill-formed end of object";
        return false;
    }

    return true;
}

int OctreeEntitiesFileParser::nextToken() {
    while (_position < _entitiesLength) {
        char c = _entitiesContents[_position++];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            return c;
        }
        if (c == '\n') {
            ++_line;
        }
    }

    return -1;
}

string OctreeEntitiesFileParser::readString() {
    string returnString;
    while (_position < _entitiesLength) {
        char c = _entitiesContents[_position++];
        if (c == '"') {
            break;
        } else {
            returnString.push_back(c);
        }
    }

    return returnString;
}

int OctreeEntitiesFileParser::readInteger() {
    const char* currentPosition = _entitiesContents.constData() + _position;
    int i = atoi(currentPosition);

    int token;
    do {
        token = nextToken();
    } while (token == '-' || token == '+' || (token >= '0' && token <= '9'));

    --_position;
    return i;
}

bool OctreeEntitiesFileParser::readEntitiesArray(QVariantList& entitiesArray) {
    if (nextToken() != '[') {
        _errorString = "Entities entry is not an array";
        return false;
    }

    while (true) {
        QJsonParseError parseError;
        QByteArray entitiesJson(_entitiesContents.right(_entitiesLength - _position));
        QJsonDocument entity = QJsonDocument::fromJson(entitiesJson, &parseError);
        if (parseError.error != QJsonParseError::GarbageAtEnd) {
            _errorString = "Ill-formed entity array";
            return false;
        }
        int entityLength = parseError.offset;
        entitiesJson.truncate(entityLength);
        _position += entityLength;

        entity = QJsonDocument::fromJson(entitiesJson, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            _errorString = "Entity item parse error";
            return false;
        }
        entitiesArray.append(entity.object());
        char c = nextToken();
        if (c == ']') {
            return true;
        } else if (c != ',') {
            _errorString = "Entity array item incorrectly terminated";
            return false;
        }
    }
    return true;
}
