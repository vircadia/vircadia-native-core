//
//  OctreeEntititesFileParser.cpp
//  libraries/octree/src
//
//  Created by Simon Walton on Oct 15, 2018.
//  Copyright 2018 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "OctreeEntitiesFileParser.h"

#include <sstream>
#include <cctype>

#include <QUuid>
#include <QJsonDocument>
#include <QJsonObject>


using std::string;

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

    int token = nextToken();

    while (true) {
        if (token == '}') {
            break;
        }
        else if (token != '"') {
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

            parsedEntities["Entities"] = std::move(entitiesValue);
            gotEntities = true;
        } else if (key == "Id") {
            if (gotId) {
                _errorString = "Duplicate Id entries";
                return false;
            }
            gotId = true;
            if (nextToken() != '"') {
                _errorString = "Invalid Id value";
                return false;
            };
            string idString = readString();
            if (idString.size() == 0) {
                _errorString = "Invalid Id string";
                return false;
            }

            // some older archives may have a null string id, so
            // return success without setting parsedEntities,
            // which will result in a new uuid for the restored
            // archive.  (not parsing and using isNull as parsing
            // results in null if there is a corrupt string)

            if (idString != "{00000000-0000-0000-0000-000000000000}") {
                QUuid idValue = QUuid::fromString(QLatin1String(idString.c_str()) );
                if (idValue.isNull()) {
                    _errorString = "Id value invalid UUID string: " + idString;
                    return false;
                }
                parsedEntities["Id"] = idValue;
            }
        } else if (key == "Version") {
            if (gotVersion) {
                _errorString = "Duplicate Version entries";
                return false;
            }

            int versionValue = readInteger();
            parsedEntities["Version"] = versionValue;
            gotVersion = true;
        } else if (key == "Paths") {
            // Serverless JSON has optional Paths entry.
            if (nextToken() != '{') {
                _errorString = "Paths item is not an object";
                return false;
            }

            int matchingBrace = findMatchingBrace();
            if (matchingBrace < 0) {
                _errorString = "Unterminated entity object";
                return false;
            }

            QByteArray jsonObject = _entitiesContents.mid(_position - 1, matchingBrace - _position + 1);
            QJsonDocument pathsObject = QJsonDocument::fromJson(jsonObject);
            if (pathsObject.isNull()) {
                _errorString = "Ill-formed paths entry";
                return false;
            }

            parsedEntities["Paths"] = pathsObject.object();
            _position = matchingBrace;
        } else {
            _errorString = "Unrecognized key name: " + key;
            return false;
        }

        token = nextToken();
        if (token == ',') {
            token = nextToken();
        }
    }

    if (nextToken() != -1) {
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
    int i = std::atoi(currentPosition);

    int token;
    do {
        token = nextToken();
    } while (token == '-' || token == '+' || std::isdigit(token));

    --_position;
    return i;
}

bool OctreeEntitiesFileParser::readEntitiesArray(QVariantList& entitiesArray) {
    if (nextToken() != '[') {
        _errorString = "Entities entry is not an array";
        return false;
    }

    while (true) {
        if (nextToken() != '{') {
            _errorString = "Entity array item is not an object";
            return false;
        }
        int matchingBrace = findMatchingBrace();
        if (matchingBrace < 0) {
            _errorString = "Unterminated entity object";
            return false;
        }

        QByteArray jsonEntity = _entitiesContents.mid(_position - 1, matchingBrace - _position + 1);
        QJsonDocument entity = QJsonDocument::fromJson(jsonEntity);
        if (entity.isNull()) {
            _errorString = "Ill-formed entity";
            return false;
        }

        QJsonObject entityObject = entity.object();

        // resolve urls starting with ./ or ../ 
        if (!_relativeURL.isEmpty()) {
            bool isDirty = false;

            const QStringList urlKeys { 
                // model
                "modelURL",
                "animation.url",
                "textures",
                // image
                "imageURL",
                // web
                "sourceUrl",
                "scriptURL",
                // zone
                "ambientLight.ambientURL",
                "skybox.url",
                // particles
                //"textures",  Already specified for model entity type.
                // materials
                "materialURL",
                // ...shared
                "href",
                "script",
                "serverScripts",
                "collisionSoundURL",
                "compoundShapeURL",
                // TODO: deal with materialData and userData
            };

            for (const QString& key : urlKeys) {
                if (key.contains('.')) {
                    // url is inside another object
                    const QStringList keyPair = key.split('.');
                    const QString entityKey = keyPair[0];
                    const QString childKey = keyPair[1];

                    if (entityObject.contains(entityKey) && entityObject[entityKey].isObject()) {
                        QJsonObject childObject = entityObject[entityKey].toObject();

                        if (childObject.contains(childKey) && childObject[childKey].isString()) {
                            const QString url = childObject[childKey].toString();

                            if (url.startsWith("./") || url.startsWith("../")) {
                                childObject[childKey] = _relativeURL.resolved(url).toString();
                                entityObject[entityKey] = childObject;
                                isDirty = true;
                            }
                        }
                    }
                } else {
                    if (entityObject.contains(key) && entityObject[key].isString()) {
                        const QString value = entityObject[key].toString();

                        if (value.startsWith("./") || value.startsWith("../")) {
                            // URL value.
                            entityObject[key] = _relativeURL.resolved(value).toString();
                            isDirty = true;
                        } else if (value.startsWith("{")) {
                            // Object with URL values.
                            auto document = QJsonDocument::fromJson(value.toUtf8());
                            if (!document.isNull()) {
                                auto object = document.object();
                                bool isObjectUpdated = false;
                                for (const QString& key : object.keys()) {
                                    auto value = object[key].toString();
                                    if (value.startsWith("./") || value.startsWith("../")) {
                                        object[key] = _relativeURL.resolved(value).toString();
                                        isObjectUpdated = true;
                                    }
                                }
                                if (isObjectUpdated) {
                                    entityObject[key] = QString(QJsonDocument(object).toJson());
                                    isDirty = true;
                                }
                            }
                        }
                    }
                }
            }

            if (isDirty) {
                entity.setObject(entityObject);
            }
        }

        entitiesArray.append(entityObject);
        _position = matchingBrace;
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

int OctreeEntitiesFileParser::findMatchingBrace() const {
    int index = _position;
    int nestCount = 1;
    while (index < _entitiesLength && nestCount != 0) {
        switch (_entitiesContents[index++]) {
        case '{':
            ++nestCount;
            break;

        case '}':
            --nestCount;
            break;

        case '"':
            // Skip string
            while (index < _entitiesLength) {
                if (_entitiesContents[index] == '"') {
                    ++index;
                    break;
                } else if (_entitiesContents[index] == '\\' && _entitiesContents[++index] == 'u') {
                    index += 4;
                }
                ++index;
            }
            break;

        default:
            break;
        }
    }

    return nestCount == 0 ? index : -1;
}
