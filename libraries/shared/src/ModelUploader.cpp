//
//  ModelUploader.cpp
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
#include <QTemporaryDir>
#include <QVariant>
#include <QMessageBox>

#include "AccountManager.h"
#include "ModelUploader.h"


static const QString NAME_FIELD = "name";
static const QString FILENAME_FIELD = "filename";
static const QString TEXDIR_FIELD = "texdir";
static const QString LOD_FIELD = "lod";

static const QString MODEL_URL = "/api/v1/models";

static const int MAX_SIZE = 10 * 1024 * 1024; // 10 MB

// Class providing the QObject parent system to QTemporaryDir
class TemporaryDir : public QTemporaryDir, public QObject {
public:
    virtual ~TemporaryDir() {
        // ensuring the entire object gets deleted by the QObject parent.
    }
};

ModelUploader::ModelUploader(bool isHead) :
    _zipDir(new TemporaryDir()),
    _lodCount(-1),
    _texturesCount(-1),
    _totalSize(0),
    _isHead(isHead),
    _readyToSend(false),
    _dataMultiPart(new QHttpMultiPart(QHttpMultiPart::FormDataType))
{
    _zipDir->setParent(_dataMultiPart);
    
}

ModelUploader::~ModelUploader() {
    delete _dataMultiPart;
}

bool ModelUploader::zip() {
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
        qDebug() << "[Warning] " << QString("Could not open FST file.");
        return false;
    }
    qDebug() << "Reading FST file : " << QFileInfo(fst).filePath();
    
    // Compress and copy the fst
    if (!compressFile(QFileInfo(fst).filePath(), _zipDir->path() + "/" + QFileInfo(fst).fileName())) {
        return false;
    }
    if (!addPart(_zipDir->path() + "/" + QFileInfo(fst).fileName(),
                 QString("fst"))) {
        return false;
    }
    
    // Let's read through the FST file
    QTextStream stream(&fst);
    QList<QString> line;
    while (!stream.atEnd()) {
        line = stream.readLine().split(QRegExp("[ =]"), QString::SkipEmptyParts);
        if (line.isEmpty()) {
            continue;
        }
        
        // according to what is read, we modify the command
        if (line[0] == NAME_FIELD) {
            QHttpPart textPart;
            textPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                               " name=\"model_name\"");
            textPart.setBody(line[1].toUtf8());
            _dataMultiPart->append(textPart);
        } else if (line[0] == FILENAME_FIELD) {
            QFileInfo fbx(QFileInfo(fst).path() + "/" + line[1]);
            if (!fbx.exists() || !fbx.isFile()) { // Check existence
                QMessageBox::warning(NULL,
                                     QString("ModelUploader::zip()"),
                                     QString("FBX file %1 could not be found.").arg(fbx.fileName()),
                                     QMessageBox::Ok);
                qDebug() << "[Warning] " << QString("FBX file %1 could not be found.").arg(fbx.fileName());
                return false;
            }
            // Compress and copy
            if (!compressFile(fbx.filePath(), _zipDir->path() + "/" + line[1])) {
                return false;
            }
            if (!addPart(_zipDir->path() + "/" + line[1], "fbx")) {
                return false;
            }
        } else if (line[0] == TEXDIR_FIELD) { // Check existence
            QFileInfo texdir(QFileInfo(fst).path() + "/" + line[1]);
            if (!texdir.exists() || !texdir.isDir()) {
                QMessageBox::warning(NULL,
                                     QString("ModelUploader::zip()"),
                                     QString("Texture directory could not be found."),
                                     QMessageBox::Ok);
                qDebug() << "[Warning] " << QString("Texture directory could not be found.");
                return false;
            }
            if (!addTextures(texdir)) { // Recursive compress and copy
                return false;
            }
        } else if (line[0] == LOD_FIELD) {
            QFileInfo lod(QFileInfo(fst).path() + "/" + line[1]);
            if (!lod.exists() || !lod.isFile()) { // Check existence
                QMessageBox::warning(NULL,
                                     QString("ModelUploader::zip()"),
                                     QString("FBX file %1 could not be found.").arg(lod.fileName()),
                                     QMessageBox::Ok);
                qDebug() << "[Warning] " << QString("FBX file %1 could not be found.").arg(lod.fileName());
                return false;
            }
            // Compress and copy
            if (!compressFile(lod.filePath(), _zipDir->path() + "/" + line[1])) {
                return false;
            }
            if (!addPart(_zipDir->path() + "/" + line[1], QString("lod%1").arg(++_lodCount))) {
                return false;
            }
        }
    }
    
    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data;"
                       " name=\"model_category\"");
    if (_isHead) {
        textPart.setBody("heads");
    } else {
        textPart.setBody("skeletons");
    }
    _dataMultiPart->append(textPart);
    
    _readyToSend = true;
    return true;
}

bool ModelUploader::send() {
    if (!_readyToSend) {
        return false;
    }
    
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "uploadSuccess";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "uploadFailed";
    
    AccountManager::getInstance().authenticatedRequest(MODEL_URL, QNetworkAccessManager::PostOperation, callbackParams, QByteArray(), _dataMultiPart);
    _zipDir = NULL;
    _dataMultiPart = NULL;
    qDebug() << "Sending model...";
    
    return true;
}

void ModelUploader::uploadSuccess(const QJsonObject& jsonResponse) {
    qDebug() << "Model sent with success to the data server.";
    qDebug() << "It might take a few minute for it to appear in your model browser.";
    deleteLater();
}

void ModelUploader::uploadFailed(QNetworkReply::NetworkError errorCode, const QString& errorString) {
    QMessageBox::warning(NULL,
                         QString("ModelUploader::uploadFailed()"),
                         QString("Model could not be sent to the data server."),
                         QMessageBox::Ok);
    qDebug() << "Model upload failed (" << errorCode << "): " << errorString;
    deleteLater();
}

bool ModelUploader::addTextures(const QFileInfo& texdir) {
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
            if (!compressFile(info.filePath(), _zipDir->path() + "/" + info.fileName())) {
                return false;
            }
            if (!addPart(_zipDir->path() + "/" + info.fileName(),
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

bool ModelUploader::compressFile(const QString &inFileName, const QString &outFileName) {
    QFile inFile(inFileName);
    inFile.open(QIODevice::ReadOnly);
    QByteArray buffer = inFile.readAll();
    
    QFile outFile(outFileName);
    if (!outFile.open(QIODevice::WriteOnly)) {
        QDir(_zipDir->path()).mkpath(QFileInfo(outFileName).path());
        if (!outFile.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(NULL,
                                 QString("ModelUploader::compressFile()"),
                                 QString("Could not compress %1").arg(inFileName),
                                 QMessageBox::Ok);
            qDebug() << "[Warning] " << QString("Could not compress %1").arg(inFileName);
            return false;
        }
    }
    QDataStream out(&outFile);
    out << qCompress(buffer);
    
    return true;
}


bool ModelUploader::addPart(const QString &path, const QString& name) {
    QFile* file = new QFile(path);
    if (!file->open(QIODevice::ReadOnly)) {
        QMessageBox::warning(NULL,
                             QString("ModelUploader::addPart()"),
                             QString("Could not open %1").arg(path),
                             QMessageBox::Ok);
        qDebug() << "[Warning] " << QString("Could not open %1").arg(path);
        delete file;
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
    
    
    qDebug() << "File " << QFileInfo(*file).fileName() << " added to model.";
    _totalSize += file->size();
    if (_totalSize > MAX_SIZE) {
        QMessageBox::warning(NULL,
                             QString("ModelUploader::zip()"),
                             QString("Model too big, over %1 Bytes.").arg(MAX_SIZE),
                             QMessageBox::Ok);
        qDebug() << "[Warning] " << QString("Model too big, over %1 Bytes.").arg(MAX_SIZE);
        return false;
    }
    qDebug() << "Current model size: " << _totalSize;
    
    return true;
}






