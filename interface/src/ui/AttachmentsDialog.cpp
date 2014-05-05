//
//  MetavoxelEditor.cpp
//  interface/src/ui
//
//  Created by Andrzej Kapolka on 1/21/14.
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
#include <QVBoxLayout>

#include "Application.h"
#include "AttachmentsDialog.h"

AttachmentsDialog::AttachmentsDialog() :
    QDialog(Application::getInstance()->getWindow()) {
    
    setWindowTitle("Edit Attachments");
    setAttribute(Qt::WA_DeleteOnClose);
    
    QVBoxLayout* layout = new QVBoxLayout();
    setLayout(layout);
    
    foreach (const AttachmentData& data, Application::getInstance()->getAvatar()->getAttachmentData()) {
        addAttachment(data);
    }
    
    QPushButton* newAttachment = new QPushButton("New Attachment");
    connect(newAttachment, SIGNAL(clicked(bool)), SLOT(addAttachment()));
    layout->addWidget(newAttachment);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    layout->addWidget(buttons);
    connect(buttons, SIGNAL(accepted()), SLOT(deleteLater()));
}

void AttachmentsDialog::addAttachment(const AttachmentData& data) {
    QVBoxLayout* layout = static_cast<QVBoxLayout*>(this->layout());
    layout->insertWidget(layout->count() - 2, new AttachmentPanel(data));
}

AttachmentPanel::AttachmentPanel(const AttachmentData& data) {
    QFormLayout* layout = new QFormLayout();
    setLayout(layout);
 
    QHBoxLayout* urlBox = new QHBoxLayout();
    layout->addRow("Model URL:", urlBox);
    urlBox->addWidget(_modelURL = new QLineEdit(data.modelURL.toString()), 1);
    QPushButton* chooseURL = new QPushButton("Choose");
    urlBox->addWidget(chooseURL);
    connect(chooseURL, SIGNAL(clicked(bool)), SLOT(chooseModelURL()));
    
    layout->addRow("Joint:", _jointName = new QComboBox());
    
    QHBoxLayout* translationBox = new QHBoxLayout();
    translationBox->addWidget(_translationX = new QDoubleSpinBox());
    translationBox->addWidget(_translationY = new QDoubleSpinBox());
    translationBox->addWidget(_translationZ = new QDoubleSpinBox());
    layout->addRow("Translation:", translationBox);
    
    QHBoxLayout* rotationBox = new QHBoxLayout();
    rotationBox->addWidget(_rotationX = new QDoubleSpinBox());
    rotationBox->addWidget(_rotationY = new QDoubleSpinBox());
    rotationBox->addWidget(_rotationZ = new QDoubleSpinBox());
    layout->addRow("Rotation:", rotationBox);
    
    layout->addRow("Scale:", _scale = new QDoubleSpinBox());
    _scale->setSingleStep(0.01);
    _scale->setMaximum(FLT_MAX);
    
    QPushButton* remove = new QPushButton("Delete");
    layout->addRow(remove);
    connect(remove, SIGNAL(clicked(bool)), SLOT(deleteLater()));
}

void AttachmentPanel::chooseModelURL() {
    ModelsBrowser modelBrowser(ATTACHMENT_MODEL, this);
    connect(&modelBrowser, SIGNAL(selected(QString)), SLOT(setModelURL(const QString&)));
    modelBrowser.browse();
}

void AttachmentPanel::setModelURL(const QString& url) {
    _modelURL->setText(url);
}
