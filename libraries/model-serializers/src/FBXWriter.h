//
//  FBXWriter.h
//  libraries/model-serializers/src
//
//  Created by Ryan Huffman on 9/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FBXWriter_h
#define hifi_FBXWriter_h

#include "FBX.h"

#include <QByteArray>
#include <QDataStream>

//#define USE_FBX_2016_FORMAT

class FBXWriter {
public:
    static QByteArray encodeFBX(const FBXNode& root);

    static void encodeNode(QDataStream& out, const FBXNode& node);
    static void encodeFBXProperty(QDataStream& out, const QVariant& property);
};

#endif // hifi_FBXWriter_h
