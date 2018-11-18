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
#include <FSTReader.h>

#include "ui/ModelsBrowser.h"

class QDoubleSpinBox;
class QComboBox;
class QCheckBox;
class QVBoxLayout;

/// A dialog that allows customization of various model properties.
class ModelPropertiesDialog : public QDialog {
    Q_OBJECT
    
public:
    ModelPropertiesDialog(const QVariantHash& originalMapping,
                          const QString& basePath, const HFMModel& hfmModel);
    
    QVariantHash getMapping() const;
    
private slots:
    void reset();
    void chooseTextureDirectory();
    void chooseScriptDirectory();
    void updatePivotJoint();
    void createNewFreeJoint(const QString& joint = QString());
    
private:
    QComboBox* createJointBox(bool withNone = true) const;
    QDoubleSpinBox* createTranslationBox() const;
    void insertJointMapping(QVariantHash& joints, const QString& joint, const QString& name) const;
    
    QVariantHash _originalMapping;
    QString _basePath;
    HFMModel _hfmModel;
    QLineEdit* _name = nullptr;
    QPushButton* _textureDirectory = nullptr;
    QPushButton* _scriptDirectory = nullptr;
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
