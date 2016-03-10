//
//  ModelPropertiesDialog.cpp
//
//
//  Created by Clement on 3/10/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>

#include <FSTReader.h>
#include <GLMHelpers.h>
#include <OffscreenUi.h>

#include "ModelPropertiesDialog.h"


ModelPropertiesDialog::ModelPropertiesDialog(FSTReader::ModelType modelType, const QVariantHash& originalMapping,
                                             const QString& basePath, const FBXGeometry& geometry) :
_modelType(modelType),
_originalMapping(originalMapping),
_basePath(basePath),
_geometry(geometry)
{
    setWindowTitle("Set Model Properties");

    QFormLayout* form = new QFormLayout();
    setLayout(form);

    form->addRow("Name:", _name = new QLineEdit());

    form->addRow("Texture Directory:", _textureDirectory = new QPushButton());
    connect(_textureDirectory, SIGNAL(clicked(bool)), SLOT(chooseTextureDirectory()));

    form->addRow("Scale:", _scale = new QDoubleSpinBox());
    _scale->setMaximum(FLT_MAX);
    _scale->setSingleStep(0.01);

    if (_modelType != FSTReader::ENTITY_MODEL) {
        if (_modelType == FSTReader::ATTACHMENT_MODEL) {
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
        if (_modelType == FSTReader::BODY_ONLY_MODEL || _modelType == FSTReader::HEAD_AND_BODY_MODEL) {
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


QString ModelPropertiesDialog::getType() const {
    return FSTReader::getNameFromType(_modelType);
}

QVariantHash ModelPropertiesDialog::getMapping() const {
    QVariantHash mapping = _originalMapping;
    mapping.insert(TYPE_FIELD, getType());
    mapping.insert(NAME_FIELD, _name->text());
    mapping.insert(TEXDIR_FIELD, _textureDirectory->text());
    mapping.insert(SCALE_FIELD, QString::number(_scale->value()));

    // update the joint indices
    QVariantHash jointIndices;
    for (int i = 0; i < _geometry.joints.size(); i++) {
        jointIndices.insert(_geometry.joints.at(i).name, QString::number(i));
    }
    mapping.insert(JOINT_INDEX_FIELD, jointIndices);

    if (_modelType != FSTReader::ENTITY_MODEL) {
        QVariantHash joints = mapping.value(JOINT_FIELD).toHash();
        if (_modelType == FSTReader::ATTACHMENT_MODEL) {
            glm::vec3 pivot;
            if (_pivotAboutCenter->isChecked()) {
                pivot = (_geometry.meshExtents.minimum + _geometry.meshExtents.maximum) * 0.5f;

            } else if (_pivotJoint->currentIndex() != 0) {
                pivot = extractTranslation(_geometry.joints.at(_pivotJoint->currentIndex() - 1).transform);
            }
            mapping.insert(TRANSLATION_X_FIELD, -pivot.x * (float)_scale->value() + (float)_translationX->value());
            mapping.insert(TRANSLATION_Y_FIELD, -pivot.y * (float)_scale->value() + (float)_translationY->value());
            mapping.insert(TRANSLATION_Z_FIELD, -pivot.z * (float)_scale->value() + (float)_translationZ->value());

        } else {
            insertJointMapping(joints, "jointEyeLeft", _leftEyeJoint->currentText());
            insertJointMapping(joints, "jointEyeRight", _rightEyeJoint->currentText());
            insertJointMapping(joints, "jointNeck", _neckJoint->currentText());
        }


        if (_modelType == FSTReader::BODY_ONLY_MODEL || _modelType == FSTReader::HEAD_AND_BODY_MODEL) {
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
    }

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

    if (_modelType != FSTReader::ENTITY_MODEL) {
        if (_modelType == FSTReader::ATTACHMENT_MODEL) {
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

        if (_modelType == FSTReader::BODY_ONLY_MODEL || _modelType == FSTReader::HEAD_AND_BODY_MODEL) {
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
}

void ModelPropertiesDialog::chooseTextureDirectory() {
    QString directory = QFileDialog::getExistingDirectory(this, "Choose Texture Directory",
                                                          _basePath + "/" + _textureDirectory->text());
    if (directory.isEmpty()) {
        return;
    }
    if (!directory.startsWith(_basePath)) {
        OffscreenUi::warning(NULL, "Invalid texture directory", "Texture directory must be child of base path.");
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

QDoubleSpinBox* ModelPropertiesDialog::createTranslationBox() const {
    QDoubleSpinBox* box = new QDoubleSpinBox();
    const double MAX_TRANSLATION = 1000000.0;
    box->setMinimum(-MAX_TRANSLATION);
    box->setMaximum(MAX_TRANSLATION);
    return box;
}

void ModelPropertiesDialog::insertJointMapping(QVariantHash& joints, const QString& joint, const QString& name) const {
    if (_geometry.jointIndices.contains(name)) {
        joints.insert(joint, name);
    } else {
        joints.remove(joint);
    }
}
