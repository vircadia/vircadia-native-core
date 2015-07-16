//
//  FileUtils.h
//  interface/src
//
//  Created by Stojce Slavkovski on 12/23/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileUtils_h
#define hifi_FileUtils_h

#include <QString>

class FileUtils {

public:
    static void locateFile(QString fileName);
    static QString standardPath(QString subfolder);

};

#endif // hifi_FileUtils_h
