//
//  OctreeEntititesFileParser.h
//  libraries/octree/src
//
//  Created by Simon Walton on Oct 15, 2018.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Parse the top-level of the Models object ourselves - use QJsonDocument for each Entity object.

#ifndef hifi_OctreeEntitiesFileParser_h
#define hifi_OctreeEntitiesFileParser_h

#include <QByteArray>
#include <QUrl>
#include <QVariant>

class OctreeEntitiesFileParser {
public:
    void setEntitiesString(const QByteArray& entitiesContents);
    void setRelativeURL(const QUrl& relativeURL) { _relativeURL = relativeURL; }
    bool parseEntities(QVariantMap& parsedEntities);
    std::string getErrorString() const;

private:
    int nextToken();
    std::string readString();
    int readInteger();
    bool readEntitiesArray(QVariantList& entitiesArray);
    int findMatchingBrace() const;

    QByteArray _entitiesContents;
    QUrl _relativeURL;
    int _position { 0 };
    int _line { 1 };
    int _entitiesLength { 0 };
    std::string _errorString;
};

#endif  // hifi_OctreeEntitiesFileParser_h
