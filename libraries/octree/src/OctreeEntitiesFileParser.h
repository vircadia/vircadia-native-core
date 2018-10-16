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

#ifndef hifi_OctreeEntitiesFileParser_h
#define hifi_OctreeEntitiesFileParser_h

#include <QVariantMap>
#include <QByteArray>

class OctreeEntitiesFileParser {
public:
    OctreeEntitiesFileParser();
    void setEntitiesString(const QByteArray& entitiesContents);
    bool parseEntities(QVariantMap& parsedEntities);
    std::string getErrorString() const;

private:
    int nextToken();
    std::string readString();
    int readInteger();
    bool readEntitiesArray(QVariantList& entitiesArray);

    QByteArray _entitiesContents;
    int _position { 0 };
    int _line { 1 };
    int _entitiesLength { 0 };
    std::string _errorString;
};

#endif  // hifi_OctreeEntitiesFileParser_h
