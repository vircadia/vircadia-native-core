//
//  FstReader.cpp
//  hifi
//
//  Created by Clément Brisset on 3/4/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileDialog>
#include <QStandardPaths>

#include "FstReader.h"

FstReader::FstReader() {
}


bool FstReader::zip() {
    // File Dialog
    QString filename = QFileDialog::getOpenFileName(NULL,
                                                    "Select your .fst file ...",
                                                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                                                    "*.fst");
    
    // First we check the FST file
    QFile fst(filename);
    if (!fst.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "[ERROR] Could not open FST file : " << fst.fileName();
        return false;
    }
    qDebug() << "Reading FST file : " << QFileInfo(fst).filePath();
    
    
    QDir rootDir(_zipDir.path());
    
    // Let's read through the FST file
    QTextStream stream(&fst);
    QList<QString> line;
    while (!stream.atEnd()) {
        line = stream.readLine().split(QRegExp("[ =]"), QString::SkipEmptyParts);
        if (line.isEmpty()) {
            continue;
        }
        
        // according to what is read, we modify the command
        if (line.first() == nameField) {
            _modelName = line[1];
        } else if (line.first() == filenameField) {
            QFileInfo fbx(QFileInfo(fst).path() + "/" + line[1]);
            if (!fbx.exists() || !fbx.isFile()) { // Check existence
                qDebug() << "[ERROR] FBX file " << fbx.absoluteFilePath() << " doesn't exist.";
                return false;
            } else { // Compress and copy
                _fbxFile = rootDir.path() + "/" + line[1];
                _totalSize += fbx.size();
                compressFile(fbx.filePath(), _fbxFile);
            }
        } else if (line.first() == texdirField) { // Check existence
            QFileInfo texdir(QFileInfo(fst).path() + "/" + line[1]);
            if (!texdir.exists() || !texdir.isDir()) {
                qDebug() << "[ERROR] Texture directory " << texdir.absolutePath() << " doesn't exist.";
                return false;
            }
            if (!addTextures(texdir)) { // Recursive compress and copy
                return false;
            }
        } else if (line.first() == lodField) {
            QFileInfo lod(QFileInfo(fst).path() + "/" + line[1]);
            if (!lod.exists() || !lod.isFile()) { // Check existence
                qDebug() << "[ERROR] FBX file " << lod.absoluteFilePath() << " doesn't exist.";
                //return false;
            } else { // Compress and copy
                _lodFiles.push_back(rootDir.path() + "/" + line[1]);
                _totalSize += lod.size();
                compressFile(lod.filePath(), _lodFiles.back());
            }
        }
    }
    
    // Compress and copy the fst
    _fstFile = rootDir.path() + "/" + QFileInfo(fst).fileName();
    _totalSize += QFileInfo(fst).size();
    compressFile(fst.fileName(), _fstFile);
    
    return true;
}

bool FstReader::send() {
    QString command = QString("curl -F \"model_name=%1\"").arg(_modelName);
    
    command += QString(" -F \"fst=@%1\" -F \"fbx=@%2\"").arg(_fstFile, _fbxFile);
    for (int i = 0; i < _lodFiles.size(); ++i) {
        command += QString(" -F \"lod%1=@%2\"").arg(i).arg(_lodFiles[i]);
    }
    for (int i = 0; i < _textureFiles.size(); ++i) {
        command += QString(" -F \"lod%1=@%2\"").arg(i).arg(_textureFiles[i]);
    }
    command += " http://localhost:3000/api/v1/models/?access_token\\=017894b7312316a2b5025613fcc58c13bc701da9b797cca34b60aae9d1c53acb --trace-ascii /dev/stdout";
    
    qDebug() << "[DEBUG] " << command;
    
    return true;
}

bool FstReader::addTextures(QFileInfo& texdir) {
    QStringList filter;
    filter << "*.png" << "*.tiff" << "*.jpg" << "*.jpeg";
    
    QFileInfoList list = QDir(texdir.filePath()).entryInfoList(filter,
                                                               QDir::Files |
                                                               QDir::AllDirs |
                                                               QDir::NoDotAndDotDot |
                                                               QDir::NoSymLinks);
    foreach (QFileInfo info, list) {
        if (info.isFile()) {
            _textureFiles.push_back(_zipDir.path() + "/" + info.fileName());
            _totalSize += info.size();
            compressFile(info.canonicalFilePath(), _textureFiles.back());
        } else if (info.isDir()) {
            if (!addTextures(info)) {
                return false;
            }
        } else {
            qDebug() << "[DEBUG] Invalid file type : " << info.filePath();
        }
    }
    
    return true;
}

bool FstReader::compressFile(const QString &inFileName, const QString &outFileName) {
    QFile inFile(inFileName);
    inFile.open(QIODevice::ReadOnly);
    QByteArray buffer = inFile.readAll();
    
    QFile outFile(outFileName);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QDir(_zipDir.path()).mkpath(QFileInfo(outFileName).path());
        if (!outFile.open(QIODevice::WriteOnly)) {
            qDebug() << "[ERROR] Could not compress " << inFileName << ".";
            return false;
        }
    }
    QDataStream out(&outFile);
    out << qCompress(buffer);
    
    return true;
}









