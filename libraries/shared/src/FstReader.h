//
//  FstReader.h
//  hifi
//
//  Created by Clément Brisset on 3/4/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__FstReader__
#define __hifi__FstReader__

#include <QString>
#include <QList>
#include <QTemporaryDir>

static const QString filenameField = "filename";
static const QString texdirField = "texdir";
static const QString lodField = "lod";

static const int MAX_FBX_SIZE = 1024 * 1024; // 1 MB
static const int MAX_TEXTURE_SIZE = 1024 * 1024; // 1 MB

class FstReader {
public:
    FstReader();
    
    bool zip();
    
private:
    QTemporaryDir _zipDir;
    
    bool addTextures(QFileInfo& texdir, QDir newTexdir);
    bool compressFile(const QString& inFileName, const QString& outFileName);
};

#endif /* defined(__hifi__FstReader__) */
