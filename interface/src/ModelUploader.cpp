//
//  ModelUploader.cpp
//  interface/src
//
//  Created by Clément Brisset on 3/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHttpMultiPart>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVariant>

#include <AccountManager.h>

#include <FBXReader.h>

#include "Application.h"
#include "ModelUploader.h"


static const QString NAME_FIELD = "name";
static const QString FILENAME_FIELD = "filename";
static const QString TEXDIR_FIELD = "texdir";
static const QString LOD_FIELD = "lod";
static const QString JOINT_INDEX_FIELD = "jointIndex";

static const QString S3_URL = "http://highfidelity-public.s3-us-west-1.amazonaws.com";
static const QString DATA_SERVER_URL = "https://data-web.highfidelity.io";
static const QString MODEL_URL = "/api/v1/models";

static const QString SETTING_NAME = "LastModelUploadLocation";

static const int MAX_SIZE = 10 * 1024 * 1024; // 10 MB
static const int TIMEOUT = 1000;
static const int MAX_CHECK = 30;

static const int QCOMPRESS_HEADER_POSITION = 0;
static const int QCOMPRESS_HEADER_SIZE = 4;

ModelUploader::ModelUploader(bool isHead) :
    _lodCount(-1),
    _texturesCount(-1),
    _totalSize(0),
    _isHead(isHead),
    _readyToSend(false),
    _dataMultiPart(new QHttpMultiPart(QHttpMultiPart::FormDataType)),
    _numberOfChecks(MAX_CHECK)
{
    connect(&_timer, SIGNAL(timeout()), SLOT(checkS3()));
}

ModelUploader::~ModelUploader() {
    delete _dataMultiPart;
}

bool ModelUploader::zip() {
    // File Dialog
    QSettings* settings = Application::getInstance()->lockSettings();
    QString lastLocation = settings->value(SETTING_NAME).toString();
    
    if (lastLocation.isEmpty()) {
       lastLocation  = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    // Temporary fix to Qt bug: http://stackoverflow.com/questions/16194475
#ifdef __APPLE__
    lastLocation.append("/model.fst");
#endif
    }
    
        
    QString filename = QFileDialog::getOpenFileName(NULL, "Select your model file ...",
        lastLocation, "Model files (*.fst *.fbx)");
    if (filename == "") {
        // If the user canceled we return.
        Application::getInstance()->unlockSettings();
        return false;
    }
    settings->setValue(SETTING_NAME, filename);
    Application::getInstance()->unlockSettings();
    
    // First we check the FST file (if any)
    QFile* fst;
    QVariantHash mapping;
    QString basePath;
    QString fbxFile;
    if (filename.toLower().endsWith(".fst")) {
        fst = new QFile(filename, this);
        if (!fst->open(QFile::ReadOnly | QFile::Text)) {
            QMessageBox::warning(NULL,
                                 QString("ModelUploader::zip()"),
                                 QString("Could not open FST file."),
                                 QMessageBox::Ok);
            qDebug() << "[Warning] " << QString("Could not open FST file.");
            return false;
        }
        qDebug() << "Reading FST file : " << QFileInfo(*fst).filePath();
        mapping = readMapping(fst->readAll());
        basePath = QFileInfo(*fst).path();
        fbxFile = basePath + "/" + mapping.value(FILENAME_FIELD).toString();
        QFileInfo fbxInfo(fbxFile);
        if (!fbxInfo.exists() || !fbxInfo.isFile()) { // Check existence
            QMessageBox::warning(NULL,
                                 QString("ModelUploader::zip()"),
                                 QString("FBX file %1 could not be found.").arg(fbxInfo.fileName()),
                                 QMessageBox::Ok);
            qDebug() << "[Warning] " << QString("FBX file %1 could not be found.").arg(fbxInfo.fileName());
            return false;
        }
    } else {
        fst = new QTemporaryFile(this);
        fbxFile = filename;
        basePath = QFileInfo(filename).path();
    }
    
    // open the fbx file
    QFile fbx(fbxFile);
    if (!fbx.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray fbxContents = fbx.readAll();
    FBXGeometry geometry = readFBX(fbxContents, QVariantHash());
    
    ModelPropertiesDialog properties(_isHead, mapping);
    if (properties.exec() == QDialog::Rejected) {
        return false;
    }
    mapping = properties.getMapping();
    
    QByteArray nameField = mapping.value(NAME_FIELD).toByteArray();
    if (!nameField.isEmpty()) {
        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"model_name\"");
        textPart.setBody(nameField);
        _dataMultiPart->append(textPart);
        _url = S3_URL + ((_isHead)? "/models/heads/" : "/models/skeletons/") + nameField + ".fst";
    } else {
        QMessageBox::warning(NULL,
                             QString("ModelUploader::zip()"),
                             QString("Model name is missing in the .fst file."),
                             QMessageBox::Ok);
        qDebug() << "[Warning] " << QString("Model name is missing in the .fst file.");
        return false;
    }
    
    QByteArray texdirField = mapping.value(TEXDIR_FIELD).toByteArray();
    QString texDir;
    if (!texdirField.isEmpty()) {
        texDir = basePath + "/" + texdirField;
        QFileInfo texInfo(texDir);
        if (!texInfo.exists() || !texInfo.isDir()) {
            QMessageBox::warning(NULL,
                                 QString("ModelUploader::zip()"),
                                 QString("Texture directory could not be found."),
                                 QMessageBox::Ok);
            qDebug() << "[Warning] " << QString("Texture directory could not be found.");
            return false;
        }
    }
    
    QVariantHash lodField = mapping.value(LOD_FIELD).toHash();
    for (QVariantHash::const_iterator it = lodField.constBegin(); it != lodField.constEnd(); it++) {
        QFileInfo lod(basePath + "/" + it.key());
        if (!lod.exists() || !lod.isFile()) { // Check existence
            QMessageBox::warning(NULL,
                                 QString("ModelUploader::zip()"),
                                 QString("LOD file %1 could not be found.").arg(lod.fileName()),
                                 QMessageBox::Ok);
            qDebug() << "[Warning] " << QString("FBX file %1 could not be found.").arg(lod.fileName());
        }
        // Compress and copy
        if (!addPart(lod.filePath(), QString("lod%1").arg(++_lodCount))) {
            return false;
        }
    }
    
    // Write out, compress and copy the fst
    if (!addPart(*fst, writeMapping(mapping), QString("fst"))) {
        return false;
    }
    
    // Compress and copy the fbx
    if (!addPart(fbx, fbxContents, "fbx")) {
        return false;
    }
                
    if (!addTextures(texDir, geometry)) {
        return false;
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

void ModelUploader::send() {
    if (!zip()) {
        deleteLater();
        return;
    }
    
    JSONCallbackParameters callbackParams;
    callbackParams.jsonCallbackReceiver = this;
    callbackParams.jsonCallbackMethod = "checkJSON";
    callbackParams.errorCallbackReceiver = this;
    callbackParams.errorCallbackMethod = "uploadFailed";
    
    AccountManager::getInstance().authenticatedRequest(MODEL_URL + "/" + QFileInfo(_url).baseName(),
                                                       QNetworkAccessManager::GetOperation,
                                                       callbackParams);
    
    qDebug() << "Sending model...";
    _progressDialog = new QDialog();
    _progressBar = new QProgressBar(_progressDialog);
    _progressBar->setRange(0, 100);
    _progressBar->setValue(0);
    
    _progressDialog->setWindowTitle("Uploading model...");
    _progressDialog->setLayout(new QGridLayout(_progressDialog));
    _progressDialog->layout()->addWidget(_progressBar);
    
    _progressDialog->exec();

    delete _progressDialog;
    _progressDialog = NULL;
    _progressBar = NULL;
}

void ModelUploader::checkJSON(const QJsonObject& jsonResponse) {
    if (jsonResponse.contains("status") && jsonResponse.value("status").toString() == "success") {
        qDebug() << "status : success";
        JSONCallbackParameters callbackParams;
        callbackParams.jsonCallbackReceiver = this;
        callbackParams.jsonCallbackMethod = "uploadSuccess";
        callbackParams.errorCallbackReceiver = this;
        callbackParams.errorCallbackMethod = "uploadFailed";
        callbackParams.updateReciever = this;
        callbackParams.updateSlot = SLOT(uploadUpdate(qint64, qint64));
        
        if (jsonResponse.contains("exists") && jsonResponse.value("exists").toBool()) {
            qDebug() << "exists : true";
            if (jsonResponse.contains("can_update") && jsonResponse.value("can_update").toBool()) {
                qDebug() << "can_update : true";
                
                AccountManager::getInstance().authenticatedRequest(MODEL_URL + "/" + QFileInfo(_url).baseName(),
                                                                   QNetworkAccessManager::PutOperation,
                                                                   callbackParams,
                                                                   QByteArray(),
                                                                   _dataMultiPart);
                _dataMultiPart = NULL;
            } else {
                qDebug() << "can_update : false";
                if (_progressDialog) {
                    _progressDialog->reject();
                }
                QMessageBox::warning(NULL,
                                     QString("ModelUploader::checkJSON()"),
                                     QString("This model already exist and is own by someone else."),
                                     QMessageBox::Ok);
                deleteLater();
            }
        } else {
            qDebug() << "exists : false";
            AccountManager::getInstance().authenticatedRequest(MODEL_URL,
                                                               QNetworkAccessManager::PostOperation,
                                                               callbackParams,
                                                               QByteArray(),
                                                               _dataMultiPart);
            _dataMultiPart = NULL;
        }
    } else {
        qDebug() << "status : failed";
        if (_progressDialog) {
            _progressDialog->reject();
        }
        QMessageBox::warning(NULL,
                             QString("ModelUploader::checkJSON()"),
                             QString("Something went wrong with the data-server."),
                             QMessageBox::Ok);
        deleteLater();
    }
}

void ModelUploader::uploadUpdate(qint64 bytesSent, qint64 bytesTotal) {
    if (_progressDialog) {
        _progressBar->setRange(0, bytesTotal);
        _progressBar->setValue(bytesSent);
    }
}

void ModelUploader::uploadSuccess(const QJsonObject& jsonResponse) {
    if (_progressDialog) {
        _progressDialog->accept();
    }
    QMessageBox::information(NULL,
                             QString("ModelUploader::uploadSuccess()"),
                             QString("We are reading your model information."),
                             QMessageBox::Ok);
    qDebug() << "Model sent with success";
    checkS3();
}

void ModelUploader::uploadFailed(QNetworkReply::NetworkError errorCode, const QString& errorString) {
    if (_progressDialog) {
        _progressDialog->reject();
    }
    qDebug() << "Model upload failed (" << errorCode << "): " << errorString;
    QMessageBox::warning(NULL,
                         QString("ModelUploader::uploadFailed()"),
                         QString("There was a problem with your upload, please try again later."),
                         QMessageBox::Ok);
    deleteLater();
}

void ModelUploader::checkS3() {
    qDebug() << "Checking S3 for " << _url;
    QNetworkRequest request(_url);
    QNetworkReply* reply = _networkAccessManager.head(request);
    connect(reply, SIGNAL(finished()), SLOT(processCheck()));
}

void ModelUploader::processCheck() {
    QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
    _timer.stop();
    
    switch (reply->error()) {
        case QNetworkReply::NoError:
            QMessageBox::information(NULL,
                                     QString("ModelUploader::processCheck()"),
                                     QString("Your model is now available in the browser."),
                                     QMessageBox::Ok);
            deleteLater();
            break;
        case QNetworkReply::ContentNotFoundError:
            if (--_numberOfChecks) {
                _timer.start(TIMEOUT);
                break;
            }
        default:
            QMessageBox::warning(NULL,
                                 QString("ModelUploader::processCheck()"),
                                 QString("We could not verify that your model was sent sucessfully\n"
                                         "but it may have. If you do not see it in the model browser, try to upload again."),
                                 QMessageBox::Ok);
            deleteLater();
            break;
    }
    
    delete reply;
}

bool ModelUploader::addTextures(const QString& texdir, const FBXGeometry& geometry) {
    foreach (FBXMesh mesh, geometry.meshes) {
        foreach (FBXMeshPart part, mesh.parts) {
            if (!part.diffuseTexture.filename.isEmpty() && part.diffuseTexture.content.isEmpty()) {
                if (!addPart(texdir + "/" + part.diffuseTexture.filename,
                             QString("texture%1").arg(++_texturesCount))) {
                    return false;
                }
            }
            if (!part.normalTexture.filename.isEmpty() && part.normalTexture.content.isEmpty()) {
                if (!addPart(texdir + "/" + part.normalTexture.filename,
                             QString("texture%1").arg(++_texturesCount))) {
                    return false;
                }
            }
        }
    }
    
    return true;
}

bool ModelUploader::addPart(const QString &path, const QString& name) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(NULL,
                             QString("ModelUploader::addPart()"),
                             QString("Could not open %1").arg(path),
                             QMessageBox::Ok);
        qDebug() << "[Warning] " << QString("Could not open %1").arg(path);
        return false;
    }
    return addPart(file, file.readAll(), name);
}

bool ModelUploader::addPart(const QFile& file, const QByteArray& contents, const QString& name) {
    QByteArray buffer = qCompress(contents);
    
    // Qt's qCompress() default compression level (-1) is the standard zLib compression.
    // Here remove Qt's custom header that prevent the data server from uncompressing the files with zLib.
    buffer.remove(QCOMPRESS_HEADER_POSITION, QCOMPRESS_HEADER_SIZE);
    
    QHttpPart part;
    part.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data;"
                   " name=\"" + name.toUtf8() +  "\";"
                   " filename=\"" + QFileInfo(file).fileName().toUtf8() +  "\""));
    part.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    part.setBody(buffer);
    _dataMultiPart->append(part);
    
    
    qDebug() << "File " << QFileInfo(file).fileName() << " added to model.";
    _totalSize += file.size();
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

ModelPropertiesDialog::ModelPropertiesDialog(bool isHead, const QVariantHash& originalMapping) :
    _originalMapping(originalMapping) {
    
    setWindowTitle("Set Model Properties");
    
    QFormLayout* form = new QFormLayout();
    setLayout(form);
    
    form->addRow("Name:", _name = new QLineEdit());
    form->addRow("Texture Directory:", _textureDirectory = new QPushButton());
    connect(_textureDirectory, SIGNAL(clicked(bool)), SLOT(chooseTextureDirectory()));
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
        QDialogButtonBox::Cancel | QDialogButtonBox::Reset);
    connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    connect(buttons->button(QDialogButtonBox::Reset), SIGNAL(clicked(bool)), SLOT(reset()));
    
    form->addRow(buttons);
    
    // reset to initialize the fields
    reset();
}

QVariantHash ModelPropertiesDialog::getMapping() const {
    QVariantHash mapping = _originalMapping;
    mapping.insert(NAME_FIELD, _name->text());
    mapping.insert(TEXDIR_FIELD, _textureDirectory->text());
    return mapping;
}

void ModelPropertiesDialog::reset() {
    _name->setText(_originalMapping.value(NAME_FIELD).toString());
    _textureDirectory->setText(_originalMapping.value(TEXDIR_FIELD).toString());
}

void ModelPropertiesDialog::chooseTextureDirectory() {
    QString directory = QFileDialog::getExistingDirectory(this, "Choose Texture Directory", _textureDirectory->text());
    if (!directory.isEmpty()) {
        _textureDirectory->setText(directory);
    }
}
