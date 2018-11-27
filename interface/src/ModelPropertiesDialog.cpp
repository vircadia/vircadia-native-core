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

#include "ModelPropertiesDialog.h"

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


ModelPropertiesDialog::ModelPropertiesDialog(const QVariantHash& originalMapping,
                                             const QString& basePath, const HFMModel& hfmModel) :
_originalMapping(originalMapping),
_basePath(basePath),
_hfmModel(hfmModel)
{
    setWindowTitle("Set Model Properties");

    QFormLayout* form = new QFormLayout();
    setLayout(form);

    form->addRow("Name:", _name = new QLineEdit());

    form->addRow("Texture Directory:", _textureDirectory = new QPushButton());
    connect(_textureDirectory, SIGNAL(clicked(bool)), SLOT(chooseTextureDirectory()));

    form->addRow("Script Directory:", _scriptDirectory = new QPushButton());
    connect(_scriptDirectory, SIGNAL(clicked(bool)), SLOT(chooseScriptDirectory()));

    form->addRow("Scale:", _scale = new QDoubleSpinBox());
    _scale->setMaximum(FLT_MAX);
    _scale->setSingleStep(0.01);

    form->addRow("Left Eye Joint:", _leftEyeJoint = createJointBox());
    form->addRow("Right Eye Joint:", _rightEyeJoint = createJointBox());
    form->addRow("Neck Joint:", _neckJoint = createJointBox());
    form->addRow("Root Joint:", _rootJoint = createJointBox());
    form->addRow("Lean Joint:", _leanJoint = createJointBox());
    form->addRow("Head Joint:", _headJoint = createJointBox());
    form->addRow("Left Hand Joint:", _leftHandJoint = createJointBox());
    form->addRow("Right Hand Joint:", _rightHandJoint = createJointBox());

    form->addRow("Free Joints:", _freeJoints = new QVBoxLayout());
    QPushButton* newFreeJoint = new QPushButton("New Free Joint");
    _freeJoints->addWidget(newFreeJoint);
    connect(newFreeJoint, SIGNAL(clicked(bool)), SLOT(createNewFreeJoint()));

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
    mapping.insert(TYPE_FIELD, FSTReader::getNameFromType(FSTReader::HEAD_AND_BODY_MODEL));
    mapping.insert(NAME_FIELD, _name->text());
    mapping.insert(TEXDIR_FIELD, _textureDirectory->text());
    mapping.insert(SCRIPT_FIELD, _scriptDirectory->text());
    mapping.insert(SCALE_FIELD, QString::number(_scale->value()));

    // update the joint indices
    QVariantHash jointIndices;
    for (int i = 0; i < _hfmModel.joints.size(); i++) {
        jointIndices.insert(_hfmModel.joints.at(i).name, QString::number(i));
    }
    mapping.insert(JOINT_INDEX_FIELD, jointIndices);

    QVariantHash joints = mapping.value(JOINT_FIELD).toHash();
    insertJointMapping(joints, "jointEyeLeft", _leftEyeJoint->currentText());
    insertJointMapping(joints, "jointEyeRight", _rightEyeJoint->currentText());
    insertJointMapping(joints, "jointNeck", _neckJoint->currentText());


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
    _scriptDirectory->setText(_originalMapping.value(SCRIPT_FIELD).toString());

    QVariantHash jointHash = _originalMapping.value(JOINT_FIELD).toHash();

    setJointText(_leftEyeJoint, jointHash.value("jointEyeLeft").toString());
    setJointText(_rightEyeJoint, jointHash.value("jointEyeRight").toString());
    setJointText(_neckJoint, jointHash.value("jointNeck").toString());

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
        if (_hfmModel.jointIndices.contains(jointName)) {
            createNewFreeJoint(jointName);
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
        OffscreenUi::asyncWarning(NULL, "Invalid texture directory", "Texture directory must be child of base path.");
        return;
    }
    _textureDirectory->setText(directory.length() == _basePath.length() ? "." : directory.mid(_basePath.length() + 1));
}

void ModelPropertiesDialog::chooseScriptDirectory() {
    QString directory = QFileDialog::getExistingDirectory(this, "Choose Script Directory",
                                                          _basePath + "/" + _scriptDirectory->text());
    if (directory.isEmpty()) {
        return;
    }
    if (!directory.startsWith(_basePath)) {
        OffscreenUi::asyncWarning(NULL, "Invalid script directory", "Script directory must be child of base path.");
        return;
    }
    _scriptDirectory->setText(directory.length() == _basePath.length() ? "." : directory.mid(_basePath.length() + 1));
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
    foreach (const HFMJoint& joint, _hfmModel.joints) {
        if (joint.isSkeletonJoint || !_hfmModel.hasSkeletonJoints) {
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
    if (_hfmModel.jointIndices.contains(name)) {
        joints.insert(joint, name);
    } else {
        joints.remove(joint);
    }
}
