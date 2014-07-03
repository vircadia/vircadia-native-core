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
    ModelUploader(ModelType type);
    ~ModelUploader();
    
public slots:
    void send();
    
private slots:
    void checkJSON(const QJsonObject& jsonResponse);
    void uploadUpdate(qint64 bytesSent, qint64 bytesTotal);
    void uploadSuccess(const QJsonObject& jsonResponse);
    void uploadFailed(QNetworkReply::NetworkError errorCode, const QString& errorString);
    void checkS3();
    void processCheck();
    
private:
    QString _url;
    QString _textureBase;
    QSet<QByteArray> _textureFilenames;
    int _lodCount;
    int _texturesCount;
    int _totalSize;
    ModelType _modelType;
    bool _readyToSend;
    
    QHttpMultiPart* _dataMultiPart;
    
    int _numberOfChecks;
    QTimer _timer;
    
    QDialog* _progressDialog;
    QProgressBar* _progressBar;
    
    
    bool zip();
    bool addTextures(const QString& texdir, const FBXGeometry& geometry);
    bool addPart(const QString& path, const QString& name, bool isTexture = false);
    bool addPart(const QFile& file, const QByteArray& contents, const QString& name, bool isTexture = false);
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
    QLineEdit* _name;
    QPushButton* _textureDirectory;
    QDoubleSpinBox* _scale;
    QDoubleSpinBox* _translationX;
    QDoubleSpinBox* _translationY;
    QDoubleSpinBox* _translationZ;
    QCheckBox* _pivotAboutCenter;
    QComboBox* _pivotJoint;
    QComboBox* _leftEyeJoint;
    QComboBox* _rightEyeJoint;
    QComboBox* _neckJoint;
    QComboBox* _rootJoint;
    QComboBox* _leanJoint;
    QComboBox* _headJoint;
    QComboBox* _leftHandJoint;
    QComboBox* _rightHandJoint;
    QVBoxLayout* _freeJoints;
};

#endif // hifi_ModelUploader_h
