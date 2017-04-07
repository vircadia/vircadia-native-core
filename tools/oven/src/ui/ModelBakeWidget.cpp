//
//  ModelBakeWidget.cpp
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/6/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <QtCore/QDir>
#include <QtCore/QDebug>

#include "ModelBakeWidget.h"

static const QString EXPORT_DIR_SETTING_KEY = "model_export_directory";
static const QString MODEL_START_DIR_SETTING_KEY = "model_search_directory";

ModelBakeWidget::ModelBakeWidget(QWidget* parent, Qt::WindowFlags flags) :
    QWidget(parent, flags),
    _exportDirectory(EXPORT_DIR_SETTING_KEY),
    _modelStartDirectory(MODEL_START_DIR_SETTING_KEY)
{
    setupUI();
}

void ModelBakeWidget::setupUI() {
    // setup a grid layout to hold everything
    QGridLayout* gridLayout = new QGridLayout;

    int rowIndex = 0;

    // setup a section to choose the file being baked
    QLabel* modelFileLabel = new QLabel("Model File");

    _modelLineEdit = new QLineEdit;

    QPushButton* chooseFileButton = new QPushButton("Browse...");
    connect(chooseFileButton, &QPushButton::clicked, this, &ModelBakeWidget::chooseFileButtonClicked);

    // add the components for the model file picker to the layout
    gridLayout->addWidget(modelFileLabel, rowIndex, 0);
    gridLayout->addWidget(_modelLineEdit, rowIndex, 1, 1, 3);
    gridLayout->addWidget(chooseFileButton, rowIndex, 4);

    // start a new row for next component
    ++rowIndex;

    // setup a section to choose the output directory
    QLabel* outputDirectoryLabel = new QLabel("Output Directory");

    _outputDirLineEdit = new QLineEdit;

    // set the current export directory to whatever was last used
    _outputDirLineEdit->setText(_exportDirectory.get());

    // whenever the output directory line edit changes, update the value in settings
    connect(_outputDirLineEdit, &QLineEdit::textChanged, this, &ModelBakeWidget::outputDirectoryChanged);

    QPushButton* chooseOutputDirectoryButton = new QPushButton("Browse...");
    connect(chooseOutputDirectoryButton, &QPushButton::clicked, this, &ModelBakeWidget::chooseOutputDirButtonClicked);

    // add the components for the output directory picker to the layout
    gridLayout->addWidget(outputDirectoryLabel, rowIndex, 0);
    gridLayout->addWidget(_outputDirLineEdit, rowIndex, 1, 1, 3);
    gridLayout->addWidget(chooseOutputDirectoryButton, rowIndex, 4);

    // start a new row for the next component
    ++rowIndex;

    // add a button that will kickoff the bake
    QPushButton* bakeButton = new QPushButton("Bake Model");
    connect(bakeButton, &QPushButton::clicked, this, &ModelBakeWidget::bakeButtonClicked);

    // add the bake button to the grid
    gridLayout->addWidget(bakeButton, rowIndex, 0, -1, -1);

    setLayout(gridLayout);
}

void ModelBakeWidget::chooseFileButtonClicked() {
    // pop a file dialog so the user can select the model file

    // if we have picked an FBX before, start in the folder that matches the last path
    // otherwise start in the home directory
    auto startDir = _modelStartDirectory.get();
    if (startDir.isEmpty()) {
        startDir = QDir::homePath();
    }

    auto selectedFile = QFileDialog::getOpenFileName(this, "Choose Model", startDir);

    if (!selectedFile.isEmpty()) {
        // set the contents of the model file text box to be the path to the selected file
        _modelLineEdit->setText(selectedFile);
        _modelStartDirectory.set(QDir(selectedFile).absolutePath());
    }
}

void ModelBakeWidget::chooseOutputDirButtonClicked() {
    // pop a file dialog so the user can select the output directory

    // if we have a previously selected output directory, use that as the initial path in the choose dialog
    // otherwise use the user's home directory
    auto startDir = _exportDirectory.get();
    if (startDir.isEmpty()) {
        startDir = QDir::homePath();
    }

    auto selectedDir = QFileDialog::getExistingDirectory(this, "Choose Output Directory", startDir);

    if (!selectedDir.isEmpty()) {
        // set the contents of the output directory text box to be the path to the directory
        _outputDirLineEdit->setText(selectedDir);
    }
}

void ModelBakeWidget::outputDirectoryChanged(const QString& newDirectory) {
    // update the export directory setting so we can re-use it next time
    _exportDirectory.set(newDirectory);
}

void ModelBakeWidget::bakeButtonClicked() {
    // make sure we have a valid output directory
    QDir outputDirectory(_outputDirLineEdit->text());

    if (!outputDirectory.exists()) {

    }

    // make sure we have a non empty URL to a model to bake
    if (_modelLineEdit->text().isEmpty()) {

    }

    // construct a URL from the path in the model file text box
    QUrl modelToBakeURL(_modelLineEdit->text());

    // if the URL doesn't have a scheme, assume it is a local file
    if (modelToBakeURL.scheme().isEmpty()) {
        modelToBakeURL.setScheme("file");
    }

    // everything seems to be in place, kick off a bake now
    _baker.reset(new FBXBaker(modelToBakeURL, outputDirectory.absolutePath()));
    _baker->start();
}
