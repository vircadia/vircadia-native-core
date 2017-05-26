//
//  DomainBakeWidget.cpp
//  tools/oven/src/ui
//
//  Created by Stephen Birarda on 4/12/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtConcurrent>

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>

#include <QtCore/QDir>
#include <QtCore/QDebug>

#include "../Oven.h"
#include "OvenMainWindow.h"

#include "DomainBakeWidget.h"

static const QString DOMAIN_NAME_SETTING_KEY = "domain_name";
static const QString EXPORT_DIR_SETTING_KEY = "domain_export_directory";
static const QString BROWSE_START_DIR_SETTING_KEY = "domain_search_directory";
static const QString DESTINATION_PATH_SETTING_KEY = "destination_path";

DomainBakeWidget::DomainBakeWidget(QWidget* parent, Qt::WindowFlags flags) :
    BakeWidget(parent, flags),
    _domainNameSetting(DOMAIN_NAME_SETTING_KEY),
    _exportDirectory(EXPORT_DIR_SETTING_KEY),
    _browseStartDirectory(BROWSE_START_DIR_SETTING_KEY),
    _destinationPathSetting(DESTINATION_PATH_SETTING_KEY)
{
    setupUI();
}

void DomainBakeWidget::setupUI() {
    // setup a grid layout to hold everything
    QGridLayout* gridLayout = new QGridLayout;

    int rowIndex = 0;

    // setup a section to enter the name of the domain being baked
    QLabel* domainNameLabel = new QLabel("Domain Name");

    _domainNameLineEdit = new QLineEdit;
    _domainNameLineEdit->setPlaceholderText("welcome");

    // set the text of the domain name from whatever was used during last bake
    if (!_domainNameSetting.get().isEmpty()) {
        _domainNameLineEdit->setText(_domainNameSetting.get());
    }

    gridLayout->addWidget(domainNameLabel);
    gridLayout->addWidget(_domainNameLineEdit, rowIndex, 1, 1, -1);

    ++rowIndex;

    // setup a section to choose the file being baked
    QLabel* entitiesFileLabel = new QLabel("Entities File");

    _entitiesFileLineEdit = new QLineEdit;
    _entitiesFileLineEdit->setPlaceholderText("File");

    QPushButton* chooseFileButton = new QPushButton("Browse...");
    connect(chooseFileButton, &QPushButton::clicked, this, &DomainBakeWidget::chooseFileButtonClicked);

    // add the components for the entities file picker to the layout
    gridLayout->addWidget(entitiesFileLabel, rowIndex, 0);
    gridLayout->addWidget(_entitiesFileLineEdit, rowIndex, 1, 1, 3);
    gridLayout->addWidget(chooseFileButton, rowIndex, 4);

    // start a new row for next component
    ++rowIndex;

    // setup a section to choose the output directory
    QLabel* outputDirectoryLabel = new QLabel("Output Directory");

    _outputDirLineEdit = new QLineEdit;

    // set the current export directory to whatever was last used
    _outputDirLineEdit->setText(_exportDirectory.get());

    // whenever the output directory line edit changes, update the value in settings
    connect(_outputDirLineEdit, &QLineEdit::textChanged, this, &DomainBakeWidget::outputDirectoryChanged);

    QPushButton* chooseOutputDirectoryButton = new QPushButton("Browse...");
    connect(chooseOutputDirectoryButton, &QPushButton::clicked, this, &DomainBakeWidget::chooseOutputDirButtonClicked);

    // add the components for the output directory picker to the layout
    gridLayout->addWidget(outputDirectoryLabel, rowIndex, 0);
    gridLayout->addWidget(_outputDirLineEdit, rowIndex, 1, 1, 3);
    gridLayout->addWidget(chooseOutputDirectoryButton, rowIndex, 4);

    // start a new row for the next component
    ++rowIndex;

    // setup a section to choose the upload prefix - the URL where baked models will be made available
    QLabel* uploadPrefixLabel = new QLabel("Destination URL Path");

    _destinationPathLineEdit = new QLineEdit;
    _destinationPathLineEdit->setPlaceholderText("http://cdn.example.com/baked-domain/");

    if (!_destinationPathSetting.get().isEmpty()) {
        _destinationPathLineEdit->setText(_destinationPathSetting.get());
    }

    gridLayout->addWidget(uploadPrefixLabel, rowIndex, 0);
    gridLayout->addWidget(_destinationPathLineEdit, rowIndex, 1, 1, -1);

    // start a new row for the next component
    ++rowIndex;

    // add a horizontal line to split the bake/cancel buttons off
    QFrame* lineFrame = new QFrame;
    lineFrame->setFrameShape(QFrame::HLine);
    lineFrame->setFrameShadow(QFrame::Sunken);
    gridLayout->addWidget(lineFrame, rowIndex, 0, 1, -1);

    // start a new row for the next component
    ++rowIndex;

    // setup a checkbox to allow re-baking of original assets
    _rebakeOriginalsCheckBox = new QCheckBox("Re-bake originals");
    gridLayout->addWidget(_rebakeOriginalsCheckBox, rowIndex, 0);

    // add a button that will kickoff the bake
    QPushButton* bakeButton = new QPushButton("Bake");
    connect(bakeButton, &QPushButton::clicked, this, &DomainBakeWidget::bakeButtonClicked);
    gridLayout->addWidget(bakeButton, rowIndex, 3);

    // add a cancel button to go back to the modes page
    QPushButton* cancelButton = new QPushButton("Cancel");
    connect(cancelButton, &QPushButton::clicked, this, &DomainBakeWidget::cancelButtonClicked);
    gridLayout->addWidget(cancelButton, rowIndex, 4);

    setLayout(gridLayout);
}

void DomainBakeWidget::chooseFileButtonClicked() {
    // pop a file dialog so the user can select the entities file

    // if we have picked an FBX before, start in the folder that matches the last path
    // otherwise start in the home directory
    auto startDir = _browseStartDirectory.get();
    if (startDir.isEmpty()) {
        startDir = QDir::homePath();
    }

    auto selectedFile = QFileDialog::getOpenFileName(this, "Choose Entities File", startDir,
                                                     "Entities File (*.json *.gz)");

    if (!selectedFile.isEmpty()) {
        // set the contents of the entities file text box to be the path to the selected file
        _entitiesFileLineEdit->setText(selectedFile);

        auto directoryOfEntitiesFile = QFileInfo(selectedFile).absolutePath();

        // save the directory containing this entities file so we can default to it next time we show the file dialog
        _browseStartDirectory.set(directoryOfEntitiesFile);
    }
}

void DomainBakeWidget::chooseOutputDirButtonClicked() {
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

void DomainBakeWidget::outputDirectoryChanged(const QString& newDirectory) {
    // update the export directory setting so we can re-use it next time
    _exportDirectory.set(newDirectory);
}

void DomainBakeWidget::bakeButtonClicked() {

    // save whatever the current domain name is in settings, we'll re-use it next time the widget is shown
    _domainNameSetting.set(_domainNameLineEdit->text());

    // save whatever the current destination path is in settings, we'll re-use it next time the widget is shown
    _destinationPathSetting.set(_destinationPathLineEdit->text());

    // make sure we have a valid output directory
    QDir outputDirectory(_outputDirLineEdit->text());

    if (!outputDirectory.exists()) {
        return;
    }

    // make sure we have a non empty URL to an entities file to bake
    if (!_entitiesFileLineEdit->text().isEmpty()) {
        // everything seems to be in place, kick off a bake for this entities file now
        auto fileToBakeURL = QUrl::fromLocalFile(_entitiesFileLineEdit->text());
        auto domainBaker = std::unique_ptr<DomainBaker> {
                new DomainBaker(fileToBakeURL, _domainNameLineEdit->text(),
                                outputDirectory.absolutePath(), _destinationPathLineEdit->text(),
                                _rebakeOriginalsCheckBox->isChecked())
        };

        // make sure we hear from the baker when it is done
        connect(domainBaker.get(), &DomainBaker::finished, this, &DomainBakeWidget::handleFinishedBaker);

        // watch the baker's progress so that we can put its progress in the results table
        connect(domainBaker.get(), &DomainBaker::bakeProgress, this, &DomainBakeWidget::handleBakerProgress);

        // move the baker to the next available Oven worker thread
        auto nextThread = qApp->getNextWorkerThread();
        domainBaker->moveToThread(nextThread);

        // kickoff the domain baker on its thread
        QMetaObject::invokeMethod(domainBaker.get(), "bake");

        // add a pending row to the results window to show that this bake is in process
        auto resultsWindow = qApp->getMainWindow()->showResultsWindow();
        auto resultsRowName = _domainNameLineEdit->text().isEmpty() ? fileToBakeURL.fileName() : _domainNameLineEdit->text();
        auto resultsRow = resultsWindow->addPendingResultRow(resultsRowName, outputDirectory);

        // keep the unique ptr to the domain baker and the index to the row representing it in the results table
        _bakers.emplace_back(std::move(domainBaker), resultsRow);
    }
}

void DomainBakeWidget::handleBakerProgress(int baked, int total) {
    if (auto baker = qobject_cast<DomainBaker*>(sender())) {
        // add the results of this bake to the results window
        auto it = std::find_if(_bakers.begin(), _bakers.end(), [baker](const BakerRowPair& value) {
            return value.first.get() == baker;
        });

        if (it != _bakers.end()) {
            auto resultRow = it->second;
            
            // grab the results window, don't force it to be brought to the top
            auto resultsWindow = qApp->getMainWindow()->showResultsWindow(false);

            int percentage = roundf(float(baked) / float(total) * 100.0f);

            auto statusString = QString("Baking - %1 of %2 - %3%").arg(baked).arg(total).arg(percentage);
            resultsWindow->changeStatusForRow(resultRow, statusString);
        }
    }
}

void DomainBakeWidget::handleFinishedBaker() {
    if (auto baker = qobject_cast<DomainBaker*>(sender())) {
        // add the results of this bake to the results window
        auto it = std::find_if(_bakers.begin(), _bakers.end(), [baker](const BakerRowPair& value) {
            return value.first.get() == baker;
        });

        if (it != _bakers.end()) {
            auto resultRow = it->second;
            auto resultsWindow = qApp->getMainWindow()->showResultsWindow();

            if (baker->hasErrors()) {
                auto errors = baker->getErrors();
                errors.removeDuplicates();

                resultsWindow->changeStatusForRow(resultRow, errors.join("\n"));
            } else if (baker->hasWarnings()) {
                auto warnings = baker->getWarnings();
                warnings.removeDuplicates();

                resultsWindow->changeStatusForRow(resultRow, warnings.join("\n"));
            } else {
                resultsWindow->changeStatusForRow(resultRow, "Success");
            }

            // remove the DomainBaker now that it has completed
            _bakers.erase(it);
        }
    }
}
