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
#include <QFileDialog>
#include <QStandardPaths>
#include <QHttpMultiPart>
#include <QVariant>

#include "AccountManager.h"

#include "FstReader.h"


static const QString NAME_FIELD = "name";
static const QString FILENAME_FIELD = "filename";
static const QString TEXDIR_FIELD = "texdir";
static const QString LOD_FIELD = "lod";

static const QString MODEL_URL = "/api/v1/models";

static const int MAX_SIZE = 10 * 1024 * 1024; // 10 MB

FstReader::FstReader() :
    _lodCount(-1),
    _texturesCount(-1),
    _readyToSend(false),
    _dataMultiPart(new QHttpMultiPart(QHttpMultiPart::FormDataType))
{
}

FstReader::~FstReader() {
    if (_dataMultiPart) {
        delete _dataMultiPart;
    }
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
    
    // Compress and copy the fst
    if (!compressFile(QFileInfo(fst).filePath(), _zipDir.path() + "/" + QFileInfo(fst).fileName())) {
        return false;
    }
    _totalSize += QFileInfo(fst).size();
    if (!addPart(_zipDir.path() + "/" + QFileInfo(fst).fileName(),
                 QString("fst"))) {
        return false;
    }
    qDebug() << "Reading FST file : " << QFileInfo(fst).filePath();
    
    // Let's read through the FST file
    QTextStream stream(&fst);
    QList<QString> line;
    while (!stream.atEnd()) {
        line = stream.readLine().split(QRegExp("[ =]"), QString::SkipEmptyParts);
        if (line.isEmpty()) {
            continue;
        }
        
        if (_totalSize > MAX_SIZE) {
            qDebug() << "[ERROR] Model too big, over " << MAX_SIZE << " Bytes.";
            return false;
        }
        
        // according to what is read, we modify the command
        if (line.first() == NAME_FIELD) {
            QHttpPart textPart;
            textPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                               " name=\"model_name\"");
            //textPart.setRawHeader("name", "\"model_name\"");
            textPart.setBody(line[1].toUtf8());
            _dataMultiPart->append(textPart);
        } else if (line.first() == FILENAME_FIELD) {
            QFileInfo fbx(QFileInfo(fst).path() + "/" + line[1]);
            if (!fbx.exists() || !fbx.isFile()) { // Check existence
                qDebug() << "[ERROR] FBX file " << fbx.absoluteFilePath() << " doesn't exist.";
                return false;
            }
            // Compress and copy
            if (!compressFile(fbx.filePath(), _zipDir.path() + "/" + line[1])) {
                return false;
            }
            _totalSize += fbx.size();
            if (!addPart(_zipDir.path() + "/" + line[1], "fbx")) {
                return false;
            }
        } else if (line.first() == TEXDIR_FIELD) { // Check existence
            QFileInfo texdir(QFileInfo(fst).path() + "/" + line[1]);
            if (!texdir.exists() || !texdir.isDir()) {
                qDebug() << "[ERROR] Texture directory " << texdir.absolutePath() << " doesn't exist.";
                return false;
            }
            if (!addTextures(texdir)) { // Recursive compress and copy
                return false;
            }
        } else if (line.first() == LOD_FIELD) {
            QFileInfo lod(QFileInfo(fst).path() + "/" + line[1]);
            if (!lod.exists() || !lod.isFile()) { // Check existence
                qDebug() << "[ERROR] FBX file " << lod.absoluteFilePath() << " doesn't exist.";
                return false;
            }
            // Compress and copy
            if (!compressFile(lod.filePath(), _zipDir.path() + "/" + line[1])) {
                return false;
            }
            _totalSize += lod.size();
            if (!addPart(_zipDir.path() + "/" + line[1], QString("lod%1").arg(++_lodCount))) {
                return false;
            }
        }
    }
    
    _readyToSend = true;
    return true;
}

bool FstReader::send() {
    if (!_readyToSend) {
        return false;
    }
    
    AccountManager::getInstance().authenticatedRequest(MODEL_URL, QNetworkAccessManager::PostOperation, JSONCallbackParameters(), QByteArray(), _dataMultiPart);
    
    _dataMultiPart = NULL;
    return true;
}

bool FstReader::addTextures(const QFileInfo& texdir) {
    QStringList filter;
    filter << "*.png" << "*.tif" << "*.jpg" << "*.jpeg";
    
    QFileInfoList list = QDir(texdir.filePath()).entryInfoList(filter,
                                                               QDir::Files |
                                                               QDir::AllDirs |
                                                               QDir::NoDotAndDotDot |
                                                               QDir::NoSymLinks);
    foreach (QFileInfo info, list) {
        if (info.isFile()) {
            // Compress and copy
            if (!compressFile(info.filePath(), _zipDir.path() + "/" + info.fileName())) {
                return false;
            }
            _totalSize += info.size();
            if (!addPart(_zipDir.path() + "/" + info.fileName(),
                         QString("texture%1").arg(++_texturesCount))) {
                return false;
            }
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


bool FstReader::addPart(const QString &path, const QString& name) {
    QFile* file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "[ERROR] Couldn't open " << file->fileName();
        return false;
    }
    
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                   " name=\"" + name.toUtf8() +  "\";"
                   " filename=\"" + QFileInfo(*file).fileName().toUtf8() +  "\"");
    part.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    part.setBodyDevice(file);
    _dataMultiPart->append(part);
    file->setParent(_dataMultiPart);
    
    qDebug() << QFileInfo(*file).fileName().toUtf8();
    
    return true;
}






