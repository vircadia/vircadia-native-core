//
//  ModelSelector.cpp
//
//
//  Created by Clement on 3/10/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDialogButtonBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QPushButton>
#include <QStandardPaths>

#include "ModelSelector.h"

static const QString AVATAR_HEAD_AND_BODY_STRING = "Avatar Body with Head";
static const QString AVATAR_ATTACHEMENT_STRING = "Avatar Attachment";
static const QString ENTITY_MODEL_STRING = "Entity Model";

ModelSelector::ModelSelector() {
    QFormLayout* form = new QFormLayout(this);
    
    setWindowTitle("Select Model");
    setLayout(form);
    
    _browseButton = new QPushButton("Browse", this);
    connect(_browseButton, &QPushButton::clicked, this, &ModelSelector::browse);
    form->addRow("Model File:", _browseButton);
    
    _modelType = new QComboBox(this);

    _modelType->addItem(AVATAR_HEAD_AND_BODY_STRING);
    _modelType->addItem(AVATAR_ATTACHEMENT_STRING);
    _modelType->addItem(ENTITY_MODEL_STRING);
    form->addRow("Model Type:", _modelType);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &ModelSelector::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(buttons);
}

QFileInfo ModelSelector::getFileInfo() const {
    return _modelFile;
}

FSTReader::ModelType ModelSelector::getModelType() const {
    QString text = _modelType->currentText();
    
if (text == AVATAR_HEAD_AND_BODY_STRING) {
        return FSTReader::HEAD_AND_BODY_MODEL;
    } else if (text == AVATAR_ATTACHEMENT_STRING) {
        return FSTReader::ATTACHMENT_MODEL;
    } else if (text == ENTITY_MODEL_STRING) {
        return FSTReader::ENTITY_MODEL;
    } 
    Q_UNREACHABLE();
}

void ModelSelector::accept() {
    if (!_modelFile.isFile()) {
        return;
    }
    QDialog::accept();
}

void ModelSelector::browse() {
    static Setting::Handle<QString> lastModelBrowseLocation("LastModelBrowseLocation",
                                                            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    QString filename = QFileDialog::getOpenFileName(NULL, "Select your model file ...",
                                                    lastModelBrowseLocation.get(),
                                                    "Model files (*.fst *.fbx)");
    QFileInfo fileInfo(filename);
    
    if (fileInfo.isFile() && fileInfo.completeSuffix().contains(QRegExp("fst|fbx|FST|FBX"))) {
        _modelFile = fileInfo;
        _browseButton->setText(fileInfo.fileName());
        lastModelBrowseLocation.set(fileInfo.path());
    }
}