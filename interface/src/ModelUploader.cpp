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

#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHttpMultiPart>
#include <QImage>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>
#include <QVBoxLayout>
#include <QVariant>

#include <AccountManager.h>

#include "Application.h"
#include "ModelUploader.h"


static const QString NAME_FIELD = "name";
static const QString FILENAME_FIELD = "filename";
static const QString TEXDIR_FIELD = "texdir";
static const QString LOD_FIELD = "lod";
static const QString JOINT_INDEX_FIELD = "jointIndex";
static const QString SCALE_FIELD = "scale";
static const QString TRANSLATION_X_FIELD = "tx";
static const QString TRANSLATION_Y_FIELD = "ty";
static const QString TRANSLATION_Z_FIELD = "tz";
static const QString JOINT_FIELD = "joint";
static const QString FREE_JOINT_FIELD = "freeJoint";
static const QString BLENDSHAPE_FIELD = "bs";

static const QString S3_URL = "http://public.highfidelity.io";
static const QString MODEL_URL = "/api/v1/models";

static const QString SETTING_NAME = "LastModelUploadLocation";

static const int MAX_SIZE = 10 * 1024 * 1024; // 10 MB
static const int MAX_TEXTURE_SIZE = 1024;
static const int TIMEOUT = 1000;
static const int MAX_CHECK = 30;

static const int QCOMPRESS_HEADER_POSITION = 0;
static const int QCOMPRESS_HEADER_SIZE = 4;

ModelUploader::ModelUploader(ModelType modelType) :
    _lodCount(-1),
    _texturesCount(-1),
    _totalSize(0),
    _modelType(modelType),
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
        fst->open(QFile::WriteOnly);
        fbxFile = filename;
        basePath = QFileInfo(filename).path();
        mapping.insert(FILENAME_FIELD, QFileInfo(filename).fileName());
    }
    
    // open the fbx file
    QFile fbx(fbxFile);
    if (!fbx.open(QIODevice::ReadOnly)) {
        return false;
    }
    QByteArray fbxContents = fbx.readAll();
    FBXGeometry geometry = readFBX(fbxContents, QVariantHash());
    
    // make sure we have some basic mappings
    if (!mapping.contains(NAME_FIELD)) {
        mapping.insert(NAME_FIELD, QFileInfo(filename).baseName());
    }
    if (!mapping.contains(TEXDIR_FIELD)) {
        mapping.insert(TEXDIR_FIELD, ".");
    }
    
    // mixamo/autodesk defaults
    if (!mapping.contains(SCALE_FIELD)) {
        mapping.insert(SCALE_FIELD, 1.0);
    }
    QVariantHash joints = mapping.value(JOINT_FIELD).toHash();
    if (!joints.contains("jointEyeLeft")) {
        joints.insert("jointEyeLeft", geometry.jointIndices.contains("jointEyeLeft") ? "jointEyeLeft" :
            (geometry.jointIndices.contains("EyeLeft") ? "EyeLeft" : "LeftEye"));
    }
    if (!joints.contains("jointEyeRight")) {
        joints.insert("jointEyeRight", geometry.jointIndices.contains("jointEyeRight") ? "jointEyeRight" :
            geometry.jointIndices.contains("EyeRight") ? "EyeRight" : "RightEye");
    }
    if (!joints.contains("jointNeck")) {
        joints.insert("jointNeck", geometry.jointIndices.contains("jointNeck") ? "jointNeck" : "Neck");
    }
    if (!joints.contains("jointRoot")) {
        joints.insert("jointRoot", "Hips");
    }
    if (!joints.contains("jointLean")) {
        joints.insert("jointLean", "Spine");
    }
    if (!joints.contains("jointHead")) {
        const char* topName = (geometry.applicationName == "mixamo.com") ? "HeadTop_End" : "HeadEnd";
        joints.insert("jointHead", geometry.jointIndices.contains(topName) ? topName : "Head");
    }
    if (!joints.contains("jointLeftHand")) {
        joints.insert("jointLeftHand", "LeftHand");
    }
    if (!joints.contains("jointRightHand")) {
        joints.insert("jointRightHand", "RightHand");
    }
    mapping.insert(JOINT_FIELD, joints);
    if (!mapping.contains(FREE_JOINT_FIELD)) {
        mapping.insertMulti(FREE_JOINT_FIELD, "LeftArm");
        mapping.insertMulti(FREE_JOINT_FIELD, "LeftForeArm");
        mapping.insertMulti(FREE_JOINT_FIELD, "RightArm");
        mapping.insertMulti(FREE_JOINT_FIELD, "RightForeArm");
    }
    
    // mixamo blendshapes
    if (!mapping.contains(BLENDSHAPE_FIELD) && geometry.applicationName == "mixamo.com") {
        QVariantHash blendshapes;
        blendshapes.insertMulti("BrowsD_L", QVariantList() << "BrowsDown_Left" << 1.0);
        blendshapes.insertMulti("BrowsD_R", QVariantList() << "BrowsDown_Right" << 1.0);
        blendshapes.insertMulti("BrowsU_C", QVariantList() << "BrowsUp_Left" << 1.0);
        blendshapes.insertMulti("BrowsU_C", QVariantList() << "BrowsUp_Right" << 1.0);
        blendshapes.insertMulti("BrowsU_L", QVariantList() << "BrowsUp_Left" << 1.0);
        blendshapes.insertMulti("BrowsU_R", QVariantList() << "BrowsUp_Right" << 1.0);
        blendshapes.insertMulti("ChinLowerRaise", QVariantList() << "Jaw_Up" << 1.0);
        blendshapes.insertMulti("ChinUpperRaise", QVariantList() << "UpperLipUp_Left" << 0.5);
        blendshapes.insertMulti("ChinUpperRaise", QVariantList() << "UpperLipUp_Right" << 0.5);
        blendshapes.insertMulti("EyeBlink_L", QVariantList() << "Blink_Left" << 1.0);
        blendshapes.insertMulti("EyeBlink_R", QVariantList() << "Blink_Right" << 1.0);
        blendshapes.insertMulti("EyeOpen_L", QVariantList() << "EyesWide_Left" << 1.0);
        blendshapes.insertMulti("EyeOpen_R", QVariantList() << "EyesWide_Right" << 1.0);
        blendshapes.insertMulti("EyeSquint_L", QVariantList() << "Squint_Left" << 1.0);
        blendshapes.insertMulti("EyeSquint_R", QVariantList() << "Squint_Right" << 1.0);
        blendshapes.insertMulti("JawFwd", QVariantList() << "JawForeward" << 1.0);
        blendshapes.insertMulti("JawLeft", QVariantList() << "JawRotateY_Left" << 0.5);
        blendshapes.insertMulti("JawOpen", QVariantList() << "MouthOpen" << 0.7);
        blendshapes.insertMulti("JawRight", QVariantList() << "Jaw_Right" << 1.0);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "JawForeward" << 0.39);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "Jaw_Down" << 0.36);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "MouthNarrow_Left" << 1.0);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "MouthNarrow_Right" << 1.0);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "MouthWhistle_NarrowAdjust_Left" << 0.5);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "MouthWhistle_NarrowAdjust_Right" << 0.5);
        blendshapes.insertMulti("LipsFunnel", QVariantList() << "TongueUp" << 1.0);
        blendshapes.insertMulti("LipsLowerClose", QVariantList() << "LowerLipIn" << 1.0);
        blendshapes.insertMulti("LipsLowerDown", QVariantList() << "LowerLipDown_Left" << 0.7);
        blendshapes.insertMulti("LipsLowerDown", QVariantList() << "LowerLipDown_Right" << 0.7);
        blendshapes.insertMulti("LipsLowerOpen", QVariantList() << "LowerLipOut" << 1.0);
        blendshapes.insertMulti("LipsPucker", QVariantList() << "MouthNarrow_Left" << 1.0);
        blendshapes.insertMulti("LipsPucker", QVariantList() << "MouthNarrow_Right" << 1.0);
        blendshapes.insertMulti("LipsUpperClose", QVariantList() << "UpperLipIn" << 1.0);
        blendshapes.insertMulti("LipsUpperOpen", QVariantList() << "UpperLipOut" << 1.0);
        blendshapes.insertMulti("LipsUpperUp", QVariantList() << "UpperLipUp_Left" << 0.7);
        blendshapes.insertMulti("LipsUpperUp", QVariantList() << "UpperLipUp_Right" << 0.7);
        blendshapes.insertMulti("MouthDimple_L", QVariantList() << "Smile_Left" << 0.25);
        blendshapes.insertMulti("MouthDimple_R", QVariantList() << "Smile_Right" << 0.25);
        blendshapes.insertMulti("MouthFrown_L", QVariantList() << "Frown_Left" << 1.0);
        blendshapes.insertMulti("MouthFrown_R", QVariantList() << "Frown_Right" << 1.0);
        blendshapes.insertMulti("MouthLeft", QVariantList() << "Midmouth_Left" << 1.0);
        blendshapes.insertMulti("MouthRight", QVariantList() << "Midmouth_Right" << 1.0);
        blendshapes.insertMulti("MouthSmile_L", QVariantList() << "Smile_Left" << 1.0);
        blendshapes.insertMulti("MouthSmile_R", QVariantList() << "Smile_Right" << 1.0);
        blendshapes.insertMulti("Puff", QVariantList() << "CheekPuff_Left" << 1.0);
        blendshapes.insertMulti("Puff", QVariantList() << "CheekPuff_Right" << 1.0);
        blendshapes.insertMulti("Sneer", QVariantList() << "NoseScrunch_Left" << 0.75);
        blendshapes.insertMulti("Sneer", QVariantList() << "NoseScrunch_Right" << 0.75);
        blendshapes.insertMulti("Sneer", QVariantList() << "Squint_Left" << 0.5);
        blendshapes.insertMulti("Sneer", QVariantList() << "Squint_Right" << 0.5);
        mapping.insert(BLENDSHAPE_FIELD, blendshapes);
    }
    
    // open the dialog to configure the rest
    ModelPropertiesDialog properties(_modelType, mapping, basePath, geometry);
    if (properties.exec() == QDialog::Rejected) {
        return false;
    }
    mapping = properties.getMapping();
    
    QByteArray nameField = mapping.value(NAME_FIELD).toByteArray();
    QString urlBase;
    if (!nameField.isEmpty()) {
        QHttpPart textPart;
        textPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"model_name\"");
        textPart.setBody(nameField);
        _dataMultiPart->append(textPart);
        urlBase = S3_URL + "/models/" + MODEL_TYPE_NAMES[_modelType] + "/" + nameField;
        _url = urlBase + ".fst";
       
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
    _textureBase = urlBase + "/textures/";
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
    textPart.setBody(MODEL_TYPE_NAMES[_modelType]);
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

void ModelUploader::checkJSON(QNetworkReply& requestReply) {
    QJsonObject jsonResponse = QJsonDocument::fromJson(requestReply.readAll()).object();
    
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

void ModelUploader::uploadSuccess(QNetworkReply& requestReply) {
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

void ModelUploader::uploadFailed(QNetworkReply& errorReply) {
    if (_progressDialog) {
        _progressDialog->reject();
    }
    qDebug() << "Model upload failed (" << errorReply.error() << "): " << errorReply.errorString();
    QMessageBox::warning(NULL,
                         QString("ModelUploader::uploadFailed()"),
                         QString("There was a problem with your upload, please try again later."),
                         QMessageBox::Ok);
    deleteLater();
}

void ModelUploader::checkS3() {
    qDebug() << "Checking S3 for " << _url;
    QNetworkRequest request(_url);
    QNetworkReply* reply = NetworkAccessManager::getInstance().head(request);
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
            Application::getInstance()->getGeometryCache()->refresh(_url);
            foreach (const QByteArray& filename, _textureFilenames) {
                Application::getInstance()->getTextureCache()->refresh(_textureBase + filename);
            }
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
            if (!part.diffuseTexture.filename.isEmpty() && part.diffuseTexture.content.isEmpty() &&
                    !_textureFilenames.contains(part.diffuseTexture.filename)) {
                if (!addPart(texdir + "/" + part.diffuseTexture.filename,
                             QString("texture%1").arg(++_texturesCount), true)) {
                    return false;
                }
                _textureFilenames.insert(part.diffuseTexture.filename);
            }
            if (!part.normalTexture.filename.isEmpty() && part.normalTexture.content.isEmpty() &&
                    !_textureFilenames.contains(part.normalTexture.filename)) {
                if (!addPart(texdir + "/" + part.normalTexture.filename,
                             QString("texture%1").arg(++_texturesCount), true)) {
                    return false;
                }
                _textureFilenames.insert(part.normalTexture.filename);
            }
            if (!part.specularTexture.filename.isEmpty() && part.specularTexture.content.isEmpty() &&
                    !_textureFilenames.contains(part.specularTexture.filename)) {
                if (!addPart(texdir + "/" + part.specularTexture.filename,
                             QString("texture%1").arg(++_texturesCount), true)) {
                    return false;
                }
                _textureFilenames.insert(part.specularTexture.filename);
            }
        }
    }
    
    return true;
}

bool ModelUploader::addPart(const QString &path, const QString& name, bool isTexture) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(NULL,
                             QString("ModelUploader::addPart()"),
                             QString("Could not open %1").arg(path),
                             QMessageBox::Ok);
        qDebug() << "[Warning] " << QString("Could not open %1").arg(path);
        return false;
    }
    return addPart(file, file.readAll(), name, isTexture);
}

bool ModelUploader::addPart(const QFile& file, const QByteArray& contents, const QString& name, bool isTexture) {
    QFileInfo fileInfo(file);
    QByteArray recodedContents = contents;
    if (isTexture) {
        QString extension = fileInfo.suffix().toLower();
        bool isJpeg = (extension == "jpg");
        bool mustRecode = !(isJpeg || extension == "png");
        QImage image = QImage::fromData(contents);
        if (image.width() > MAX_TEXTURE_SIZE || image.height() > MAX_TEXTURE_SIZE) {
            image = image.scaled(MAX_TEXTURE_SIZE, MAX_TEXTURE_SIZE, Qt::KeepAspectRatio);
            mustRecode = true;
        }
        if (mustRecode) {
            QBuffer buffer;
            buffer.open(QIODevice::WriteOnly); 
            image.save(&buffer, isJpeg ? "JPG" : "PNG");
            recodedContents = buffer.data();
        }
    }
    QByteArray buffer = qCompress(recodedContents);
    
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
    _totalSize += recodedContents.size();
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

static QDoubleSpinBox* createTranslationBox() {
    QDoubleSpinBox* box = new QDoubleSpinBox();
    const double MAX_TRANSLATION = 1000000.0;
    box->setMinimum(-MAX_TRANSLATION);
    box->setMaximum(MAX_TRANSLATION);
    return box;
}

ModelPropertiesDialog::ModelPropertiesDialog(ModelType modelType, const QVariantHash& originalMapping,
        const QString& basePath, const FBXGeometry& geometry) :
    _modelType(modelType),
    _originalMapping(originalMapping),
    _basePath(basePath),
    _geometry(geometry) {
    
    setWindowTitle("Set Model Properties");
    
    QFormLayout* form = new QFormLayout();
    setLayout(form);
    
    form->addRow("Name:", _name = new QLineEdit());
    
    form->addRow("Texture Directory:", _textureDirectory = new QPushButton());
    connect(_textureDirectory, SIGNAL(clicked(bool)), SLOT(chooseTextureDirectory()));
    
    form->addRow("Scale:", _scale = new QDoubleSpinBox());
    _scale->setMaximum(FLT_MAX);
    _scale->setSingleStep(0.01);
    
    if (_modelType == ATTACHMENT_MODEL) {
        QHBoxLayout* translation = new QHBoxLayout();
        form->addRow("Translation:", translation);
        translation->addWidget(_translationX = createTranslationBox());
        translation->addWidget(_translationY = createTranslationBox());
        translation->addWidget(_translationZ = createTranslationBox());
        form->addRow("Pivot About Center:", _pivotAboutCenter = new QCheckBox());
        form->addRow("Pivot Joint:", _pivotJoint = createJointBox());    
        connect(_pivotAboutCenter, SIGNAL(toggled(bool)), SLOT(updatePivotJoint()));
        _pivotAboutCenter->setChecked(true);
        
    } else {
        form->addRow("Left Eye Joint:", _leftEyeJoint = createJointBox());
        form->addRow("Right Eye Joint:", _rightEyeJoint = createJointBox());
        form->addRow("Neck Joint:", _neckJoint = createJointBox());
    }
    if (_modelType == SKELETON_MODEL) {
        form->addRow("Root Joint:", _rootJoint = createJointBox());
        form->addRow("Lean Joint:", _leanJoint = createJointBox());
        form->addRow("Head Joint:", _headJoint = createJointBox());
        form->addRow("Left Hand Joint:", _leftHandJoint = createJointBox());
        form->addRow("Right Hand Joint:", _rightHandJoint = createJointBox());
        
        form->addRow("Free Joints:", _freeJoints = new QVBoxLayout());
        QPushButton* newFreeJoint = new QPushButton("New Free Joint");
        _freeJoints->addWidget(newFreeJoint);
        connect(newFreeJoint, SIGNAL(clicked(bool)), SLOT(createNewFreeJoint()));
    }
    
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
    mapping.insert(SCALE_FIELD, QString::number(_scale->value()));
    
    // update the joint indices
    QVariantHash jointIndices;
    for (int i = 0; i < _geometry.joints.size(); i++) {
        jointIndices.insert(_geometry.joints.at(i).name, QString::number(i));
    }
    mapping.insert(JOINT_INDEX_FIELD, jointIndices);
    
    QVariantHash joints = mapping.value(JOINT_FIELD).toHash();
    if (_modelType == ATTACHMENT_MODEL) {
        glm::vec3 pivot;
        if (_pivotAboutCenter->isChecked()) {
            pivot = (_geometry.meshExtents.minimum + _geometry.meshExtents.maximum) * 0.5f;
        
        } else if (_pivotJoint->currentIndex() != 0) {
            pivot = extractTranslation(_geometry.joints.at(_pivotJoint->currentIndex() - 1).transform);
        }
        mapping.insert(TRANSLATION_X_FIELD, -pivot.x * _scale->value() + _translationX->value());
        mapping.insert(TRANSLATION_Y_FIELD, -pivot.y * _scale->value() + _translationY->value());
        mapping.insert(TRANSLATION_Z_FIELD, -pivot.z * _scale->value() + _translationZ->value());
        
    } else {
        insertJointMapping(joints, "jointEyeLeft", _leftEyeJoint->currentText());
        insertJointMapping(joints, "jointEyeRight", _rightEyeJoint->currentText());
        insertJointMapping(joints, "jointNeck", _neckJoint->currentText());
    }
    if (_modelType == SKELETON_MODEL) {
        insertJointMapping(joints, "jointRoot", _rootJoint->currentText());
        insertJointMapping(joints, "jointLean", _leanJoint->currentText());
        insertJointMapping(joints, "jointHead", _headJoint->currentText());
        insertJointMapping(joints, "jointLeftHand", _leftHandJoint->currentText());
        insertJointMapping(joints, "jointRightHand", _rightHandJoint->currentText());
        
        mapping.remove(FREE_JOINT_FIELD);
        for (int i = 0; i < _freeJoints->count() - 1; i++) {
            QComboBox* box = static_cast<QComboBox*>(_freeJoints->itemAt(i)->widget()->layout()->itemAt(0)->widget());
            mapping.insertMulti(FREE_JOINT_FIELD, box->currentText());
        }
    }
    mapping.insert(JOINT_FIELD, joints);
    
    return mapping;
}

static void setJointText(QComboBox* box, const QString& text) {
    box->setCurrentIndex(qMax(box->findText(text), 0));
}

void ModelPropertiesDialog::reset() {
    _name->setText(_originalMapping.value(NAME_FIELD).toString());
    _textureDirectory->setText(_originalMapping.value(TEXDIR_FIELD).toString());
    _scale->setValue(_originalMapping.value(SCALE_FIELD).toDouble());
    
    QVariantHash jointHash = _originalMapping.value(JOINT_FIELD).toHash();
    if (_modelType == ATTACHMENT_MODEL) {
        _translationX->setValue(_originalMapping.value(TRANSLATION_X_FIELD).toDouble());
        _translationY->setValue(_originalMapping.value(TRANSLATION_Y_FIELD).toDouble());
        _translationZ->setValue(_originalMapping.value(TRANSLATION_Z_FIELD).toDouble());    
        _pivotAboutCenter->setChecked(true);
        _pivotJoint->setCurrentIndex(0);
        
    } else {
        setJointText(_leftEyeJoint, jointHash.value("jointEyeLeft").toString());
        setJointText(_rightEyeJoint, jointHash.value("jointEyeRight").toString());
        setJointText(_neckJoint, jointHash.value("jointNeck").toString());
    }
    if (_modelType == SKELETON_MODEL) {
        setJointText(_rootJoint, jointHash.value("jointRoot").toString());
        setJointText(_leanJoint, jointHash.value("jointLean").toString());
        setJointText(_headJoint, jointHash.value("jointHead").toString());
        setJointText(_leftHandJoint, jointHash.value("jointLeftHand").toString());
        setJointText(_rightHandJoint, jointHash.value("jointRightHand").toString());
        
        while (_freeJoints->count() > 1) {
            delete _freeJoints->itemAt(0)->widget();
        }
        foreach (const QVariant& joint, _originalMapping.values(FREE_JOINT_FIELD)) {
            QString jointName = joint.toString();
            if (_geometry.jointIndices.contains(jointName)) {
                createNewFreeJoint(jointName);
            }
        }
    }
}

void ModelPropertiesDialog::chooseTextureDirectory() {
    QString directory = QFileDialog::getExistingDirectory(this, "Choose Texture Directory",
        _basePath + "/" + _textureDirectory->text());
    if (directory.isEmpty()) {
        return;
    }
    if (!directory.startsWith(_basePath)) {
        QMessageBox::warning(NULL, "Invalid texture directory", "Texture directory must be child of base path.");
        return;
    }
    _textureDirectory->setText(directory.length() == _basePath.length() ? "." : directory.mid(_basePath.length() + 1));
}

void ModelPropertiesDialog::updatePivotJoint() {
    _pivotJoint->setEnabled(!_pivotAboutCenter->isChecked());
}

void ModelPropertiesDialog::createNewFreeJoint(const QString& joint) {
    QWidget* freeJoint = new QWidget();
    QHBoxLayout* freeJointLayout = new QHBoxLayout();
    freeJointLayout->setContentsMargins(QMargins());
    freeJoint->setLayout(freeJointLayout);
    QComboBox* jointBox = createJointBox(false);
    jointBox->setCurrentText(joint);
    freeJointLayout->addWidget(jointBox, 1);
    QPushButton* deleteJoint = new QPushButton("Delete");
    freeJointLayout->addWidget(deleteJoint);
    freeJoint->connect(deleteJoint, SIGNAL(clicked(bool)), SLOT(deleteLater()));
    _freeJoints->insertWidget(_freeJoints->count() - 1, freeJoint);
}

QComboBox* ModelPropertiesDialog::createJointBox(bool withNone) const {
    QComboBox* box = new QComboBox();
    if (withNone) {
        box->addItem("(none)");
    }
    foreach (const FBXJoint& joint, _geometry.joints) {
        if (joint.isSkeletonJoint || !_geometry.hasSkeletonJoints) {
            box->addItem(joint.name);
        }
    }
    return box;
}

void ModelPropertiesDialog::insertJointMapping(QVariantHash& joints, const QString& joint, const QString& name) const {
    if (_geometry.jointIndices.contains(name)) {
        joints.insert(joint, name);
    } else {
        joints.remove(joint);
    }
}

