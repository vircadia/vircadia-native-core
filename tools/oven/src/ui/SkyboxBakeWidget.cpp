//
//  SkyboxBakeWidget.cpp
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/17/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SkyboxBakeWidget.h"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QStackedWidget>

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QThread>

#include <TextureBaker.h>

#include "../OvenGUIApplication.h"

static const auto EXPORT_DIR_SETTING_KEY = "skybox_export_directory";
static const auto SELECTION_START_DIR_SETTING_KEY = "skybox_search_directory";

SkyboxBakeWidget::SkyboxBakeWidget(QWidget* parent, Qt::WindowFlags flags) :
    BakeWidget(parent, flags),
    _exportDirectory(EXPORT_DIR_SETTING_KEY),
    _selectionStartDirectory(SELECTION_START_DIR_SETTING_KEY)
{
    setupUI();
}

void SkyboxBakeWidget::setupUI() {
    // setup a grid layout to hold everything
    QGridLayout* gridLayout = new QGridLayout;

    int rowIndex = 0;

    // setup a section to choose the file being baked
    QLabel* skyboxFileLabel = new QLabel("Skybox File(s)");

    _selectionLineEdit = new QLineEdit;
    _selectionLineEdit->setPlaceholderText("File or URL");

    QPushButton* chooseFileButton = new QPushButton("Browse...");
    connect(chooseFileButton, &QPushButton::clicked, this, &SkyboxBakeWidget::chooseFileButtonClicked);

    // add the components for the skybox file picker to the layout
    gridLayout->addWidget(skyboxFileLabel, rowIndex, 0);
    gridLayout->addWidget(_selectionLineEdit, rowIndex, 1, 1, 3);
    gridLayout->addWidget(chooseFileButton, rowIndex, 4);

    // start a new row for next component
    ++rowIndex;

    // setup a section to enable Ambient map baking
    _ambientMapBox = new QCheckBox("Bake ambient map(s)");
    _ambientMapBox->setChecked(false);

    gridLayout->addWidget(_ambientMapBox, rowIndex, 1);

    // start a new row for next component
    ++rowIndex;

    // setup a section to choose the output directory
    QLabel* outputDirectoryLabel = new QLabel("Output Directory");

    _outputDirLineEdit = new QLineEdit;

    // set the current export directory to whatever was last used
    _outputDirLineEdit->setText(_exportDirectory.get());

    // whenever the output directory line edit changes, update the value in settings
    connect(_outputDirLineEdit, &QLineEdit::textChanged, this, &SkyboxBakeWidget::outputDirectoryChanged);

    QPushButton* chooseOutputDirectoryButton = new QPushButton("Browse...");
    connect(chooseOutputDirectoryButton, &QPushButton::clicked, this, &SkyboxBakeWidget::chooseOutputDirButtonClicked);

    // add the components for the output directory picker to the layout
    gridLayout->addWidget(outputDirectoryLabel, rowIndex, 0);
    gridLayout->addWidget(_outputDirLineEdit, rowIndex, 1, 1, 3);
    gridLayout->addWidget(chooseOutputDirectoryButton, rowIndex, 4);

    // start a new row for the next component
    ++rowIndex;

    // add a horizontal line to split the bake/cancel buttons off
    QFrame* lineFrame = new QFrame;
    lineFrame->setFrameShape(QFrame::HLine);
    lineFrame->setFrameShadow(QFrame::Sunken);
    gridLayout->addWidget(lineFrame, rowIndex, 0, 1, -1);

    // start a new row for the next component
    ++rowIndex;

    // add a button that will kickoff the bake
    QPushButton* bakeButton = new QPushButton("Bake");
    connect(bakeButton, &QPushButton::clicked, this, &SkyboxBakeWidget::bakeButtonClicked);
    gridLayout->addWidget(bakeButton, rowIndex, 3);

    // add a cancel button to go back to the modes page
    QPushButton* cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &SkyboxBakeWidget::cancelButtonClicked);
    gridLayout->addWidget(cancelButton, rowIndex, 4);

    setLayout(gridLayout);
}

void SkyboxBakeWidget::chooseFileButtonClicked() {
    // pop a file dialog so the user can select the skybox file(s)

    // if we have picked a skybox before, start in the folder that matches the last path
    // otherwise start in the home directory
    auto startDir = _selectionStartDirectory.get();
    if (startDir.isEmpty()) {
        startDir = QDir::homePath();
    }

    auto selectedFiles = QFileDialog::getOpenFileNames(this, "Choose Skybox", startDir);

    if (!selectedFiles.isEmpty()) {
        // set the contents of the file select text box to be the path to the selected file
        _selectionLineEdit->setText(selectedFiles.join(','));

        if (_outputDirLineEdit->text().isEmpty()) {
            auto directoryOfSkybox = QFileInfo(selectedFiles[0]).absolutePath();

            // if our output directory is not yet set, set it to the directory of this skybox
            _outputDirLineEdit->setText(directoryOfSkybox);
        }
    }
}

void SkyboxBakeWidget::chooseOutputDirButtonClicked() {
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

void SkyboxBakeWidget::outputDirectoryChanged(const QString& newDirectory) {
    // update the export directory setting so we can re-use it next time
    _exportDirectory.set(newDirectory);
}

void SkyboxBakeWidget::bakeButtonClicked() {
    // make sure we have a valid output directory
    QDir outputDirectory(_outputDirLineEdit->text());

    outputDirectory.mkdir(".");
    if (!outputDirectory.exists()) {
        QMessageBox::warning(this, "Unable to create directory", "Unable to create output directory. Please create it manually or choose a different directory.");
        return;
    }

    // make sure we have a non empty URL to a skybox to bake
    if (_selectionLineEdit->text().isEmpty()) {
        return;
    }

    // split the list from the selection line edit to see how many skyboxes we need to bake
    auto fileURLStrings = _selectionLineEdit->text().split(',');
    foreach (QString fileURLString, fileURLStrings) {
        // construct a URL from the path in the skybox file text box
        QUrl skyboxToBakeURL(fileURLString);

        // if the URL doesn't have a scheme, assume it is a local file
        if (skyboxToBakeURL.scheme() != "http" && skyboxToBakeURL.scheme() != "https" && skyboxToBakeURL.scheme() != "ftp") {
            skyboxToBakeURL = QUrl::fromLocalFile(fileURLString);
        }

        // everything seems to be in place, kick off a bake for this skybox now
        addBaker(new TextureBaker(skyboxToBakeURL, image::TextureUsage::SKY_TEXTURE, outputDirectory.absolutePath()),
                 outputDirectory);

        if (_ambientMapBox->isChecked()) {
            QString ambientMapBaseFilename;
            QString urlPath = skyboxToBakeURL.path();
            auto urlParts = urlPath.split('.');

            urlParts.front() += "-ambient";
            ambientMapBaseFilename = QUrl(urlParts.front()).fileName();

            // we need to bake the corresponding ambient map too
            addBaker(new TextureBaker(skyboxToBakeURL, image::TextureUsage::AMBIENT_TEXTURE, outputDirectory.absolutePath(), QString(), ambientMapBaseFilename),
                     outputDirectory);
        }
    }
}

void SkyboxBakeWidget::addBaker(TextureBaker* baker, const QDir& outputDirectory) {
    auto textureBaker = std::unique_ptr<TextureBaker>{ baker };

    // move the textureBaker to a worker thread
    textureBaker->moveToThread(Oven::instance().getNextWorkerThread());

    // make sure we hear about the results of this textureBaker when it is done
    connect(textureBaker.get(), &TextureBaker::finished, this, &SkyboxBakeWidget::handleFinishedBaker);

    // invoke the bake method on the textureBaker thread
    QMetaObject::invokeMethod(textureBaker.get(), "bake");

    // add a pending row to the results window to show that this bake is in process
    auto resultsWindow = OvenGUIApplication::instance()->getMainWindow()->showResultsWindow();
    auto resultsRow = resultsWindow->addPendingResultRow(baker->getBaseFilename(), outputDirectory);

    // keep a unique_ptr to this textureBaker
    // and remember the row that represents it in the results table
    _bakers.emplace_back(std::move(textureBaker), resultsRow);
}

void SkyboxBakeWidget::handleFinishedBaker() {
    if (auto textureBaker = qobject_cast<TextureBaker*>(sender())) {
        // add the results of this bake to the results window
        auto it = std::find_if(_bakers.begin(), _bakers.end(), [textureBaker](const BakerRowPair& value) {
            return value.first.get() == textureBaker;
        });

        if (it != _bakers.end()) {
            auto resultRow = it->second;
            auto resultsWindow = OvenGUIApplication::instance()->getMainWindow()->showResultsWindow();

            if (textureBaker->hasErrors()) {
                resultsWindow->changeStatusForRow(resultRow, textureBaker->getErrors().join("\n"));
            } else {
                resultsWindow->changeStatusForRow(resultRow, "Success");
            }

            // drop our strong pointer to the textureBaker now that we are done with it
            _bakers.erase(it);
        }
    }
}
