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
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QMessageBox>

#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QThread>

#include "../Oven.h"
#include "../OvenGUIApplication.h"
#include "OvenMainWindow.h"
#include "FBXBaker.h"
#include "OBJBaker.h"
#include "ModelBakeWidget.h"


static const auto EXPORT_DIR_SETTING_KEY = "model_export_directory";
static const auto MODEL_START_DIR_SETTING_KEY = "model_search_directory";

ModelBakeWidget::ModelBakeWidget(QWidget* parent, Qt::WindowFlags flags) :
    BakeWidget(parent, flags),
    _exportDirectory(EXPORT_DIR_SETTING_KEY),
    _modelStartDirectory(MODEL_START_DIR_SETTING_KEY) {
    setupUI();
}

void ModelBakeWidget::setupUI() {
    // setup a grid layout to hold everything
    QGridLayout* gridLayout = new QGridLayout;

    int rowIndex = 0;

    // setup a section to choose the file being baked
    QLabel* modelFileLabel = new QLabel("Model File(s)");

    _modelLineEdit = new QLineEdit;
    _modelLineEdit->setPlaceholderText("File or URL");

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

    // add a horizontal line to split the bake/cancel buttons off
    QFrame* lineFrame = new QFrame;
    lineFrame->setFrameShape(QFrame::HLine);
    lineFrame->setFrameShadow(QFrame::Sunken);
    gridLayout->addWidget(lineFrame, rowIndex, 0, 1, -1);

    // start a new row for the next component
    ++rowIndex;

    // add a button that will kickoff the bake
    QPushButton* bakeButton = new QPushButton("Bake");
    connect(bakeButton, &QPushButton::clicked, this, &ModelBakeWidget::bakeButtonClicked);
    gridLayout->addWidget(bakeButton, rowIndex, 3);

    // add a cancel button to go back to the modes page
    QPushButton* cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &ModelBakeWidget::cancelButtonClicked);
    gridLayout->addWidget(cancelButton, rowIndex, 4);

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

    auto selectedFiles = QFileDialog::getOpenFileNames(this, "Choose Model", startDir, "Models (*.fbx *.obj)");

    if (!selectedFiles.isEmpty()) {
        // set the contents of the model file text box to be the path to the selected file
        _modelLineEdit->setText(selectedFiles.join(','));

        auto directoryOfModel = QFileInfo(selectedFiles[0]).absolutePath();

        if (_outputDirLineEdit->text().isEmpty()) {
            // if our output directory is not yet set, set it to the directory of this model
            _outputDirLineEdit->setText(directoryOfModel);
        }

        // save the directory containing the file(s) so we can default to it next time we show the file dialog
        _modelStartDirectory.set(directoryOfModel);
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

    // make sure we have a non empty URL to a model to bake
    if (_modelLineEdit->text().isEmpty()) {
        QMessageBox::warning(this, "Model URL unspecified", "A model file is required.");
        return;
    }

    // split the list from the model line edit to see how many models we need to bake
    auto fileURLStrings = _modelLineEdit->text().split(',');
    foreach (QString fileURLString, fileURLStrings) {
        // construct a URL from the path in the model file text box
        QUrl modelToBakeURL(fileURLString);

        // if the URL doesn't have a scheme, assume it is a local file
        if (modelToBakeURL.scheme() != "http" && modelToBakeURL.scheme() != "https" && modelToBakeURL.scheme() != "ftp") {
            qDebug() << modelToBakeURL.toString();
            qDebug() << modelToBakeURL.scheme();
            modelToBakeURL = QUrl::fromLocalFile(fileURLString);
            qDebug() << "New url: " << modelToBakeURL;
        }

        auto modelName = modelToBakeURL.fileName().left(modelToBakeURL.fileName().lastIndexOf('.'));

        // make sure we have a valid output directory
        QDir outputDirectory(_outputDirLineEdit->text());
        QString subFolderName = modelName + "/";

        // output in a sub-folder with the name of the fbx, potentially suffixed by a number to make it unique
        int iteration = 0;

        while (outputDirectory.exists(subFolderName)) {
            subFolderName = modelName + "-" + QString::number(++iteration) + "/";
        }

        outputDirectory.mkpath(subFolderName);

        if (!outputDirectory.exists()) {
            QMessageBox::warning(this, "Unable to create directory", "Unable to create output directory. Please create it manually or choose a different directory.");
            return;
        }

        outputDirectory.cd(subFolderName);

        QDir bakedOutputDirectory = outputDirectory.absoluteFilePath("baked");
        QDir originalOutputDirectory = outputDirectory.absoluteFilePath("original");

        bakedOutputDirectory.mkdir(".");
        originalOutputDirectory.mkdir(".");

        std::unique_ptr<Baker> baker;
        auto getWorkerThreadCallback = []() -> QThread* {
            return Oven::instance().getNextWorkerThread();
        };
        // everything seems to be in place, kick off a bake for this model now
        if (modelToBakeURL.fileName().endsWith(".fbx")) {
            baker.reset(new FBXBaker(modelToBakeURL, getWorkerThreadCallback, bakedOutputDirectory.absolutePath(),
                        originalOutputDirectory.absolutePath()));
        } else if (modelToBakeURL.fileName().endsWith(".obj")) {
            baker.reset(new OBJBaker(modelToBakeURL, getWorkerThreadCallback, bakedOutputDirectory.absolutePath(),
                        originalOutputDirectory.absolutePath()));
        } else {
            qWarning() << "Unknown model type: " << modelToBakeURL.fileName();
            continue;
        }

        // move the baker to the FBX baker thread
        baker->moveToThread(Oven::instance().getNextWorkerThread());

        // invoke the bake method on the baker thread
        QMetaObject::invokeMethod(baker.get(), "bake");

        // make sure we hear about the results of this baker when it is done
        connect(baker.get(), &Baker::finished, this, &ModelBakeWidget::handleFinishedBaker);

        // add a pending row to the results window to show that this bake is in process
        auto resultsWindow = OvenGUIApplication::instance()->getMainWindow()->showResultsWindow();
        auto resultsRow = resultsWindow->addPendingResultRow(modelToBakeURL.fileName(), outputDirectory);

        // keep a unique_ptr to this baker
        // and remember the row that represents it in the results table
        _bakers.emplace_back(std::move(baker), resultsRow);
    }
}

void ModelBakeWidget::handleFinishedBaker() {
    Baker* baker = dynamic_cast<Baker*>(sender());
    if (!baker) {
        qWarning() << "Received signal from unexpected sender";
        return;
    }

    // add the results of this bake to the results window
    auto it = std::find_if(_bakers.begin(), _bakers.end(), [baker](const BakerRowPair& value) {
        return value.first.get() == baker;
    });

    for (auto& file : baker->getOutputFiles()) {
        qDebug() << "Baked file: " << file;
    }

    if (it != _bakers.end()) {
        auto resultRow = it->second;
        auto resultsWindow = OvenGUIApplication::instance()->getMainWindow()->showResultsWindow();

        if (baker->hasErrors()) {
            resultsWindow->changeStatusForRow(resultRow, baker->getErrors().join("\n"));
        } else {
            resultsWindow->changeStatusForRow(resultRow, "Success");
        }

        _bakers.erase(it);
    }
}
