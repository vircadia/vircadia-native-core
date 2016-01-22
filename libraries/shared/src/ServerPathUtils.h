//
//  ServerPathUtils.h
//  libraries/shared/src
//
//  Created by Ryan Huffman on 01/12/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ServerPathUtils_h
#define hifi_ServerPathUtils_h

#include <QString>

namespace ServerPathUtils {
    QString getDataDirectory();
    QString getDataFilePath(QString filename);
}

#endif // hifi_ServerPathUtils_h