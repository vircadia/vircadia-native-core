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


static const QString nameField = "name";
static const QString filenameField = "filename";
static const QString texdirField = "texdir";
static const QString lodField = "lod";

static const int MAX_SIZE = 1024 * 1024; // 1 MB

class FstReader {
public:
    FstReader();
    
    bool zip();
    bool send();
    
private:
    QTemporaryDir _zipDir;
    
    QString _modelName;
    QString _fstFile;
    QString _fbxFile;
    QStringList _lodFiles;
    QStringList _textureFiles;
    
    int _totalSize;
    
    
    bool addTextures(QFileInfo& texdir);
    bool compressFile(const QString& inFileName, const QString& outFileName);
};

#endif /* defined(__hifi__FstReader__) */
