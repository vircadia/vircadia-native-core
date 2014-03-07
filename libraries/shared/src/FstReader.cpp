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
    
    
    QTemporaryDir tempRootDir(_zipDir.path() + "/" + QFileInfo(fst).baseName());
    QDir rootDir(tempRootDir.path());
    
    // Let's read through the FST file
    QTextStream stream(&fst);
    QList<QString> line;
    while (!stream.atEnd()) {
        line = stream.readLine().split(QRegExp("[ =]"), QString::SkipEmptyParts);
        if (line.isEmpty()) {
            continue;
        }
        
        // according to what is read, we modify the command
        if (line.first() == filenameField) {
            QFileInfo fbx(QFileInfo(fst).path() + "/" + line.at(1));
            if (!fbx.exists() || !fbx.isFile()) { // Check existence
                qDebug() << "[ERROR] FBX file " << fbx.absoluteFilePath() << " doesn't exist.";
                return false;
            } else if (fbx.size() > MAX_FBX_SIZE) { // Check size
                qDebug() << "[ERROR] FBX file " << fbx.absoluteFilePath() << " too big, over " << MAX_FBX_SIZE << " MB.";
                return false;
            } else { // Compress and copy
                compressFile(fbx.filePath(),
                             rootDir.path() + "/" + line.at(1));
            }
        } else if (line.first() == texdirField) { // Check existence
            QFileInfo texdir(QFileInfo(fst).path() + "/" + line.at(1));
            if (!texdir.exists() || !texdir.isDir()) {
                qDebug() << "[ERROR] Texture directory " << texdir.absolutePath() << " doesn't exist.";
                return false;
            }
            QDir newTexdir(rootDir.canonicalPath() + "/" + line.at(1));
            if (!newTexdir.exists() && !rootDir.mkpath(line.at(1))) { // Create texdir
                qDebug() << "[ERROR] Couldn't create " << line.at(1) << ".";
                return false;
            }
            if (!addTextures(texdir, newTexdir)) { // Recursive compress and copy
                return false;
            }
        } else if (line.first() == lodField) {
            QFileInfo lod(QFileInfo(fst).path() + "/" + line.at(1));
            if (!lod.exists() || !lod.isFile()) { // Check existence
                qDebug() << "[ERROR] FBX file " << lod.absoluteFilePath() << " doesn't exist.";
                return false;
            } else if (lod.size() > MAX_FBX_SIZE) { // Check size
                qDebug() << "[ERROR] FBX file " << lod.absoluteFilePath() << " too big, over " << MAX_FBX_SIZE << " MB.";\
                return false;
            } else { // Compress and copy
                compressFile(lod.filePath(), rootDir.path() + "/" + line.at(1));
            }
        }
    }
    
    // Compress and copy the fst
    compressFile(fst.fileName(),
                 rootDir.path() + "/" + QFileInfo(fst).fileName());
    
    tempRootDir.setAutoRemove(false);
    
    return true;
}

bool FstReader::addTextures(QFileInfo& texdir, QDir newTexdir) {
    QStringList filter;
    filter << "*.png" << "*.tiff" << "*.jpg" << "*.jpeg";
    
    QFileInfoList list = QDir(texdir.filePath()).entryInfoList(filter,
                                                               QDir::Files |
                                                               QDir::AllDirs |
                                                               QDir::NoDotAndDotDot |
                                                               QDir::NoSymLinks);
    foreach (QFileInfo info, list) {
        if (info.isFile()) {
            if (info.size() > MAX_TEXTURE_SIZE) {
                qDebug() << "[ERROR] Texture " << info.absoluteFilePath()
                         << "too big, file  over " << MAX_TEXTURE_SIZE << " Bytes.";
                return false;
            }
            compressFile(info.canonicalFilePath(), newTexdir.path() + "/" + info.fileName());
        } else if (info.isDir()) {
            if (newTexdir.mkdir(info.fileName())) {
                qDebug() << "[ERROR] Couldn't create texdir.";
                return false;
            }
            QDir texdirChild(newTexdir.canonicalPath() + "/" + info.fileName());
            if (!addTextures(info, QDir(info.canonicalFilePath()))) {
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









