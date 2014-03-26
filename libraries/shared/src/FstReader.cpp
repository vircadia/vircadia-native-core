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
#include <QMessageBox>

#include "AccountManager.h"

#include "FstReader.h"


static const QString NAME_FIELD = "name";
static const QString FILENAME_FIELD = "filename";
static const QString TEXDIR_FIELD = "texdir";
static const QString LOD_FIELD = "lod";
static const QString HEAD_SPECIFIC_FIELD = "bs";

static const QString MODEL_URL = "/api/v1/models";

static const int MAX_SIZE = 10 * 1024 * 1024; // 10 MB

FstReader::FstReader() :
    _lodCount(-1),
    _texturesCount(-1),
    _isHead(false),
    _readyToSend(false),
    _dataMultiPart(new QHttpMultiPart(QHttpMultiPart::FormDataType))
{
}

FstReader::~FstReader() {
    delete _dataMultiPart;
}

bool FstReader::zip() {
    // File Dialog
    QString filename = QFileDialog::getOpenFileName(NULL,
                                                    "Select your .fst file ...",
                                                    QStandardPaths::writableLocation(QStandardPaths::DownloadLocation),
                                                    "*.fst");
    if (filename == "") {
        // If the user canceled we return.
        return false;
    }
    
    // First we check the FST file
    QFile fst(filename);
    if (!fst.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(NULL,
                             QString("ModelUploader::zip()"),
                             QString("Could not open FST file."),
                             QMessageBox::Ok);
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
            QMessageBox::warning(NULL,
                                 QString("ModelUploader::zip()"),
                                 QString("Model too big, over %1 Bytes.").arg(MAX_SIZE),
                                 QMessageBox::Ok);
            return false;
        }
        
        // according to what is read, we modify the command
        if (line[1] == HEAD_SPECIFIC_FIELD) {
            _isHead = true;
        } else if (line[1] == NAME_FIELD) {
            QHttpPart textPart;
            textPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                               " name=\"model_name\"");
            textPart.setBody(line[1].toUtf8());
            _dataMultiPart->append(textPart);
        } else if (line[1] == FILENAME_FIELD) {
            QFileInfo fbx(QFileInfo(fst).path() + "/" + line[1]);
            if (!fbx.exists() || !fbx.isFile()) { // Check existence
                QMessageBox::warning(NULL,
                                     QString("ModelUploader::zip()"),
                                     QString("FBX file %1 could not be found.").arg(fbx.fileName()),
                                     QMessageBox::Ok);
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
        } else if (line[1] == TEXDIR_FIELD) { // Check existence
            QFileInfo texdir(QFileInfo(fst).path() + "/" + line[1]);
            if (!texdir.exists() || !texdir.isDir()) {
                QMessageBox::warning(NULL,
                                     QString("ModelUploader::zip()"),
                                     QString("Texture directory could not be found."),
                                     QMessageBox::Ok);
                return false;
            }
            if (!addTextures(texdir)) { // Recursive compress and copy
                return false;
            }
        } else if (line[1] == LOD_FIELD) {
            QFileInfo lod(QFileInfo(fst).path() + "/" + line[1]);
            if (!lod.exists() || !lod.isFile()) { // Check existence
                QMessageBox::warning(NULL,
                                     QString("ModelUploader::zip()"),
                                     QString("FBX file %1 could not be found.").arg(lod.fileName()),
                                     QMessageBox::Ok);
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
    
    
    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                       " name=\"model_category\"");
    if (_isHead) {
        textPart.setBody("head");
    } else {
        textPart.setBody("skeleton");
    }
    _dataMultiPart->append(textPart);
    
    _readyToSend = true;
    return true;
}

bool FstReader::send() {
    if (!_readyToSend) {
        return false;
    }
    
    AccountManager::getInstance().authenticatedRequest(MODEL_URL, QNetworkAccessManager::PostOperation, JSONCallbackParameters(), QByteArray(), _dataMultiPart);
    
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
            QMessageBox::warning(NULL,
                                 QString("ModelUploader::compressFile()"),
                                 QString("Could not compress %1").arg(inFileName),
                                 QMessageBox::Ok);
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
        QMessageBox::warning(NULL,
                             QString("ModelUploader::addPart()"),
                             QString("Could not open %1").arg(path),
                             QMessageBox::Ok);
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
    
    return true;
}






