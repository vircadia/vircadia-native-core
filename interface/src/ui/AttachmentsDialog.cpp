//
//  AttachmentsDialog.cpp
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 5/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "Application.h"
#include "AttachmentsDialog.h"
#include "MainWindow.h"

AttachmentsDialog::AttachmentsDialog() :
    QDialog(Application::getInstance()->getWindow()) {
    
    setWindowTitle("Edit Attachments");
    setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout* layout = new QVBoxLayout();
    setLayout(layout);
    
    QScrollArea* area = new QScrollArea();
    layout->addWidget(area);
    area->setWidgetResizable(true);
    QWidget* container = new QWidget();
    container->setLayout(_attachments = new QVBoxLayout());
    container->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    area->setWidget(container);
    _attachments->addStretch(1);
    
    foreach (const AttachmentData& data, Application::getInstance()->getAvatar()->getAttachmentData()) {
        addAttachment(data);
    }
    
    QPushButton* newAttachment = new QPushButton("New Attachment");
    connect(newAttachment, SIGNAL(clicked(bool)), SLOT(addAttachment()));
    layout->addWidget(newAttachment);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    layout->addWidget(buttons);
    connect(buttons, SIGNAL(accepted()), SLOT(deleteLater()));
    _ok = buttons->button(QDialogButtonBox::Ok);
    
    setMinimumSize(600, 600);
}

void AttachmentsDialog::setVisible(bool visible) {
    QDialog::setVisible(visible);
    
    // un-default the OK button
    if (visible) {
        _ok->setDefault(false);
    }
}

void AttachmentsDialog::updateAttachmentData() {
    QVector<AttachmentData> data;
    for (int i = 0; i < _attachments->count() - 1; i++) {
        data.append(static_cast<AttachmentPanel*>(_attachments->itemAt(i)->widget())->getAttachmentData());
    }
    Application::getInstance()->getAvatar()->setAttachmentData(data);
}

void AttachmentsDialog::addAttachment(const AttachmentData& data) {
    _attachments->insertWidget(_attachments->count() - 1, new AttachmentPanel(this, data));
}

static QDoubleSpinBox* createTranslationBox(AttachmentPanel* panel, float value) {
    QDoubleSpinBox* box = new QDoubleSpinBox();
    box->setSingleStep(0.01);
    box->setMinimum(-FLT_MAX);
    box->setMaximum(FLT_MAX);
    box->setValue(value);
    panel->connect(box, SIGNAL(valueChanged(double)), SLOT(updateAttachmentData()));
    return box;
}

static QDoubleSpinBox* createRotationBox(AttachmentPanel* panel, float value) {
    QDoubleSpinBox* box = new QDoubleSpinBox();
    box->setMinimum(-180.0);
    box->setMaximum(180.0);
    box->setWrapping(true);
    box->setValue(value);
    panel->connect(box, SIGNAL(valueChanged(double)), SLOT(updateAttachmentData()));
    return box;
}

AttachmentPanel::AttachmentPanel(AttachmentsDialog* dialog, const AttachmentData& data) :
        _dialog(dialog),
        _applying(false) {
    setFrameStyle(QFrame::StyledPanel);
 
    QFormLayout* layout = new QFormLayout();
    layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    setLayout(layout);
 
    QHBoxLayout* urlBox = new QHBoxLayout();
    layout->addRow("Model URL:", urlBox);
    urlBox->addWidget(_modelURL = new QLineEdit(data.modelURL.toString()), 1);
    _modelURL->setText(data.modelURL.toString());
    connect(_modelURL, SIGNAL(returnPressed()), SLOT(modelURLChanged()));
    QPushButton* chooseURL = new QPushButton("Choose");
    urlBox->addWidget(chooseURL);
    connect(chooseURL, SIGNAL(clicked(bool)), SLOT(chooseModelURL()));
    
    layout->addRow("Joint:", _jointName = new QComboBox());
    QSharedPointer<NetworkGeometry> geometry = Application::getInstance()->getAvatar()->getSkeletonModel().getGeometry();
    if (geometry && geometry->isLoaded()) {
        foreach (const FBXJoint& joint, geometry->getFBXGeometry().joints) {
            _jointName->addItem(joint.name);
        }
    }
    _jointName->setCurrentText(data.jointName);
    connect(_jointName, SIGNAL(currentIndexChanged(int)), SLOT(jointNameChanged()));
    
    QHBoxLayout* translationBox = new QHBoxLayout();
    translationBox->addWidget(_translationX = createTranslationBox(this, data.translation.x));
    translationBox->addWidget(_translationY = createTranslationBox(this, data.translation.y));
    translationBox->addWidget(_translationZ = createTranslationBox(this, data.translation.z));
    layout->addRow("Translation:", translationBox);
    
    QHBoxLayout* rotationBox = new QHBoxLayout();
    glm::vec3 eulers = glm::degrees(safeEulerAngles(data.rotation));
    rotationBox->addWidget(_rotationX = createRotationBox(this, eulers.x));
    rotationBox->addWidget(_rotationY = createRotationBox(this, eulers.y));
    rotationBox->addWidget(_rotationZ = createRotationBox(this, eulers.z));
    layout->addRow("Rotation:", rotationBox);
    
    layout->addRow("Scale:", _scale = new QDoubleSpinBox());
    _scale->setSingleStep(0.01);
    _scale->setMaximum(FLT_MAX);
    _scale->setValue(data.scale);
    connect(_scale, SIGNAL(valueChanged(double)), SLOT(updateAttachmentData()));
    
    QPushButton* remove = new QPushButton("Delete");
    layout->addRow(remove);
    connect(remove, SIGNAL(clicked(bool)), SLOT(deleteLater()));
    dialog->connect(remove, SIGNAL(clicked(bool)), SLOT(updateAttachmentData()), Qt::QueuedConnection);
}

AttachmentData AttachmentPanel::getAttachmentData() const {
    AttachmentData data;
    data.modelURL = _modelURL->text();
    data.jointName = _jointName->currentText();
    data.translation = glm::vec3(_translationX->value(), _translationY->value(), _translationZ->value());
    data.rotation = glm::quat(glm::radians(glm::vec3(_rotationX->value(), _rotationY->value(), _rotationZ->value())));
    data.scale = _scale->value();
    return data;
}

void AttachmentPanel::chooseModelURL() {
    ModelsBrowser modelBrowser(ATTACHMENT_MODEL, this);
    connect(&modelBrowser, SIGNAL(selected(QString)), SLOT(setModelURL(const QString&)));
    modelBrowser.browse();
}

void AttachmentPanel::setModelURL(const QString& url) {
    _modelURL->setText(url);
    modelURLChanged();
}

void AttachmentPanel::modelURLChanged() {
    // check for saved attachment data
    if (_modelURL->text().isEmpty()) {
        _dialog->updateAttachmentData();
        return;
    }
    AttachmentData attachment = Application::getInstance()->getAvatar()->loadAttachmentData(_modelURL->text());
    if (attachment.isValid()) {
        _applying = true;
        _jointName->setCurrentText(attachment.jointName);
        applyAttachmentData(attachment);
    }
    _dialog->updateAttachmentData();
}

void AttachmentPanel::jointNameChanged() {
    if (_applying) {
        return;
    }
    // check for saved attachment data specific to this joint
    if (_modelURL->text().isEmpty()) {
        _dialog->updateAttachmentData();
        return;
    }
    AttachmentData attachment = Application::getInstance()->getAvatar()->loadAttachmentData(
        _modelURL->text(), _jointName->currentText());
    if (attachment.isValid()) {
        applyAttachmentData(attachment);
    }
    updateAttachmentData();
}

void AttachmentPanel::updateAttachmentData() {
    if (_applying) {
        return;
    }
    // save the attachment data under the model URL (if any)
    if (!_modelURL->text().isEmpty()) {
        Application::getInstance()->getAvatar()->saveAttachmentData(getAttachmentData());
    }
    _dialog->updateAttachmentData();
}

void AttachmentPanel::applyAttachmentData(const AttachmentData& attachment) {
    _applying = true;
    _translationX->setValue(attachment.translation.x);
    _translationY->setValue(attachment.translation.y);
    _translationZ->setValue(attachment.translation.z);
    glm::vec3 eulers = glm::degrees(safeEulerAngles(attachment.rotation));
    _rotationX->setValue(eulers.x);
    _rotationY->setValue(eulers.y);
    _rotationZ->setValue(eulers.z);
    _scale->setValue(attachment.scale);
    _applying = false;
    _dialog->updateAttachmentData();
}
