//
//  ModelUploader.h
//  interface/src
//
//  Created by Clément Brisset on 3/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelUploader_h
#define hifi_ModelUploader_h

#include <QDialog>
#include <QTimer>

#include <FBXReader.h>
#include <SettingHandle.h>

#include "ui/ModelsBrowser.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QFileInfo;
class QHttpMultiPart;
class QLineEdit;
class QProgressBar;
class QPushButton;
class QVBoxLayout;

class ModelUploader : public QObject {
    Q_OBJECT
    
public:
    static void uploadModel(ModelType modelType);
    
    static void uploadHead();
    static void uploadSkeleton();
    static void uploadAttachment();
    static void uploadEntity();
    
private slots:
    void send();
    void checkJSON(QNetworkReply& requestReply);
    void uploadUpdate(qint64 bytesSent, qint64 bytesTotal);
    void uploadSuccess(QNetworkReply& requestReply);
    void uploadFailed(QNetworkReply& errorReply);
    void checkS3();
    void processCheck();
    
private:
    ModelUploader(ModelType type);
    ~ModelUploader();
    
    void populateBasicMapping(QVariantHash& mapping, QString filename, const FBXGeometry& geometry);
    bool zip();
    bool addTextures(const QString& texdir, const FBXGeometry& geometry);
    bool addPart(const QString& path, const QString& name, bool isTexture = false);
    bool addPart(const QFile& file, const QByteArray& contents, const QString& name, bool isTexture = false);
    
    QString _url;
    QString _textureBase;
    QSet<QByteArray> _textureFilenames;
    int _lodCount;
    int _texturesCount;
    unsigned long _totalSize;
    ModelType _modelType;
    bool _readyToSend;
    
    QHttpMultiPart* _dataMultiPart = nullptr;
    
    int _numberOfChecks;
    QTimer _timer;
    
    QDialog* _progressDialog = nullptr;
    QProgressBar* _progressBar = nullptr;
    
    static Setting::Handle<QString> _lastModelUploadLocation;
};

/// A dialog that allows customization of various model properties.
class ModelPropertiesDialog : public QDialog {
    Q_OBJECT

public:
    ModelPropertiesDialog(ModelType modelType, const QVariantHash& originalMapping,
        const QString& basePath, const FBXGeometry& geometry);

    QVariantHash getMapping() const;

private slots:
    void reset();
    void chooseTextureDirectory();
    void updatePivotJoint();
    void createNewFreeJoint(const QString& joint = QString());

private:
    QComboBox* createJointBox(bool withNone = true) const;
    void insertJointMapping(QVariantHash& joints, const QString& joint, const QString& name) const;

    ModelType _modelType;
    QVariantHash _originalMapping;
    QString _basePath;
    FBXGeometry _geometry;
    QLineEdit* _name = nullptr;
    QPushButton* _textureDirectory = nullptr;
    QDoubleSpinBox* _scale = nullptr;
    QDoubleSpinBox* _translationX = nullptr;
    QDoubleSpinBox* _translationY = nullptr;
    QDoubleSpinBox* _translationZ = nullptr;
    QCheckBox* _pivotAboutCenter = nullptr;
    QComboBox* _pivotJoint = nullptr;
    QComboBox* _leftEyeJoint = nullptr;
    QComboBox* _rightEyeJoint = nullptr;
    QComboBox* _neckJoint = nullptr;
    QComboBox* _rootJoint = nullptr;
    QComboBox* _leanJoint = nullptr;
    QComboBox* _headJoint = nullptr;
    QComboBox* _leftHandJoint = nullptr;
    QComboBox* _rightHandJoint = nullptr;
    QVBoxLayout* _freeJoints = nullptr;
};

#endif // hifi_ModelUploader_h
