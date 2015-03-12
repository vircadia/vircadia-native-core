//
//  ModelPropertiesDialog.h
//
//
//  Created by Clement on 3/10/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelPropertiesDialog_h
#define hifi_ModelPropertiesDialog_h

#include <QDialog>

#include <FBXReader.h>

#include "ui/ModelsBrowser.h"

class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QVBoxLayout;

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
    QDoubleSpinBox* createTranslationBox() const;
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

#endif // hifi_ModelPropertiesDialog_h