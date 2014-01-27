//
//  FileUtils.h
//  hifi
//
//  Created by Stojce Slavkovski on 12/23/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef hifi_FileUtils_h
#define hifi_FileUtils_h

#include <QString>

class FileUtils {

public:
    static void locateFile(QString fileName);
    static QString standardPath(QString subfolder);

};

#endif
