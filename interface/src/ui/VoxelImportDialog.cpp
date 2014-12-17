//
//  VoxelImportDialog.cpp
//  interface/src/ui
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "InterfaceConfig.h"

#include <QStandardPaths>
#include <QGridLayout>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

#include <PathUtils.h>

#include "Application.h"

#include "VoxelImportDialog.h"
#include "voxels/VoxelImporter.h"

const QString SETTINGS_GROUP_NAME = "VoxelImport";
const QString IMPORT_DIALOG_SETTINGS_KEY = "VoxelImportDialogSettings";

const QString WINDOW_NAME = QObject::tr("Import Voxels");
const QString IMPORT_BUTTON_NAME = QObject::tr("Import Voxels");
const QString LOADING_BUTTON_NAME = QObject::tr("Loading ...");
const QString PLACE_BUTTON_NAME = QObject::tr("Place voxels");
const QString IMPORT_INFO = QObject::tr("<b>Import</b> %1 as voxels");
const QString CANCEL_BUTTON_NAME = QObject::tr("Cancel");

const QString DOWNLOAD_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
const int SHORT_FILE_EXTENSION = 4;
const int SECOND_INDEX_LETTER = 1;

QIcon HiFiIconProvider::icon(QFileIconProvider::IconType type) const {

    // types
    // Computer, Desktop, Trashcan, Network, Drive, Folder, File
    QString typeString;

    switch (type) {
        case QFileIconProvider::Computer:
            typeString = "computer";
            break;

        case QFileIconProvider::Desktop:
            typeString = "desktop";
            break;

        case QFileIconProvider::Trashcan:
        case QFileIconProvider::Network:
        case QFileIconProvider::Drive:
        case QFileIconProvider::Folder:
            typeString = "folder";
            break;

        default:
            typeString = "file";
            break;
    }

    return QIcon(PathUtils::resourcesPath() + "icons/" + typeString + ".svg");
}

QIcon HiFiIconProvider::icon(const QFileInfo &info) const {
    const QString ext = info.suffix().toLower();

    if (info.isDir()) {
        if (info.absoluteFilePath() == QDir::homePath()) {
            return QIcon(PathUtils::resourcesPath() + "icons/home.svg");
        } else if (info.absoluteFilePath() == QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)) {
            return QIcon(PathUtils::resourcesPath() + "icons/desktop.svg");
        } else if (info.absoluteFilePath() == QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)) {
            return QIcon(PathUtils::resourcesPath() + "icons/documents.svg");
        }
        return QIcon(PathUtils::resourcesPath() + "icons/folder.svg");
    }

    QFileInfo iconFile(PathUtils::resourcesPath() + "icons/" + iconsMap[ext]);
    if (iconFile.exists() && iconFile.isFile()) {
        return QIcon(iconFile.filePath());
    }

    return QIcon(PathUtils::resourcesPath() + "icons/file.svg");
}

QString HiFiIconProvider::type(const QFileInfo &info) const {
    if (info.isFile()) {
        if (info.suffix().size() > SHORT_FILE_EXTENSION) {
            // Capitalize extension
            return info.suffix().left(SECOND_INDEX_LETTER).toUpper() + info.suffix().mid(SECOND_INDEX_LETTER);
        }
        return info.suffix().toUpper();
    }

    return QFileIconProvider::type(info);
}

VoxelImportDialog::VoxelImportDialog(QWidget* parent) :
    QFileDialog(parent, WINDOW_NAME, DOWNLOAD_LOCATION, NULL),
    _cancelButton(CANCEL_BUTTON_NAME, this),
    _importButton(IMPORT_BUTTON_NAME, this),
    _importer(Application::getInstance()->getVoxelImporter()),
    _mode(importMode),
    _progressBar(this),
    _didImport(false) {

    setOption(QFileDialog::DontUseNativeDialog, true);
    setFileMode(QFileDialog::ExistingFile);
    setViewMode(QFileDialog::Detail);

    setImportTypes();
    setLayout();

    _progressBar.setRange(0, 100);
    
    connect(&_importButton, SIGNAL(pressed()), this, SLOT(accept()));
    connect(&_cancelButton, SIGNAL(pressed()), this, SLOT(cancel()));
    connect(this, SIGNAL(currentChanged(QString)), SLOT(saveCurrentFile(QString)));
}

void VoxelImportDialog::cancel() {
    switch (getMode()) {
        case importMode:
            _importer->cancel();
            close();
            break;
        default:
            _importer->reset();
            setMode(importMode);
            break;
    }
    emit canceled();
}

void VoxelImportDialog::saveSettings(QSettings* settings) {
    settings->beginGroup(SETTINGS_GROUP_NAME);
    settings->setValue(IMPORT_DIALOG_SETTINGS_KEY, saveState());
    settings->endGroup();
}

void VoxelImportDialog::loadSettings(QSettings* settings) {
    settings->beginGroup(SETTINGS_GROUP_NAME);
    restoreState(settings->value(IMPORT_DIALOG_SETTINGS_KEY).toByteArray());
    settings->endGroup();
}

bool VoxelImportDialog::prompt() {
    reset();
    exec();
    return _didImport;
}

void VoxelImportDialog::reset() {
    setMode(importMode);
    _didImport = false;
}

void VoxelImportDialog::setMode(dialogMode mode) {
    dialogMode previousMode = _mode;
    _mode = mode;
    
    switch (_mode) {
        case importMode:
            _progressBar.setValue(0);
            _importButton.setEnabled(true);
            _importButton.setText(IMPORT_BUTTON_NAME);
            findChild<QWidget*>("sidebar")->setEnabled(true);
            findChild<QWidget*>("treeView")->setEnabled(true);
            findChild<QWidget*>("backButton")->setEnabled(true);
            findChild<QWidget*>("forwardButton")->setEnabled(true);
            findChild<QWidget*>("toParentButton")->setEnabled(true);
            break;
        case loadingMode:
            // Connect to VoxelImporter signals
            connect(_importer, SIGNAL(importProgress(int)), this, SLOT(updateProgressBar(int)));
            connect(_importer, SIGNAL(importDone()), this, SLOT(afterImport()));

            _importButton.setEnabled(false);
            _importButton.setText(LOADING_BUTTON_NAME);
            findChild<QWidget*>("sidebar")->setEnabled(false);
            findChild<QWidget*>("treeView")->setEnabled(false);
            findChild<QWidget*>("backButton")->setEnabled(false);
            findChild<QWidget*>("forwardButton")->setEnabled(false);
            findChild<QWidget*>("toParentButton")->setEnabled(false);
            break;
        case finishedMode:
            if (previousMode == loadingMode) {
                // Disconnect from VoxelImporter signals
                disconnect(_importer, SIGNAL(importProgress(int)), this, SLOT(setProgressBarValue(int)));
                disconnect(_importer, SIGNAL(importDone()), this, SLOT(afterImport()));
            }
            setMode(importMode);
            break;
    }
}

void VoxelImportDialog::setProgressBarValue(int value) {
    _progressBar.setValue(value);
}

void VoxelImportDialog::accept() {
    if (getMode() == importMode) {
        QString filename = getCurrentFile();

        // If file is invalid we ignore the call
        if (!_importer->validImportFile(filename)) {
            return;
        }
        // Let's prepare the dialog window for import
        setMode(loadingMode);

        _importer->import(filename);
    }
}

void VoxelImportDialog::afterImport() {
    setMode(finishedMode);
    _didImport = true;
    close();
}

void VoxelImportDialog::saveCurrentFile(QString filename) {
    _currentFile = QFileInfo(filename).isFile() ? filename : "";
}

void VoxelImportDialog::setLayout() {
    QGridLayout* gridLayout = (QGridLayout*) layout();
    gridLayout->addWidget(&_progressBar, 2, 0, 2, 1);
    gridLayout->addWidget(&_cancelButton, 2, 1, 2, 1);
    gridLayout->addWidget(&_importButton, 2, 2, 2, 1);
    
    // set ObjectName used in qss for styling
    _progressBar.setObjectName("progressBar");
    _importButton.setObjectName("importButton");
    _cancelButton.setObjectName("cancelButton");

    // set fixed size
    _importButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _cancelButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _cancelButton.setFlat(true);
    int progressBarHeight = 7;
    _progressBar.setFixedHeight(progressBarHeight);
    _progressBar.setTextVisible(false);
    
    QGridLayout* subLayout = new QGridLayout();
    subLayout->addWidget(findChild<QWidget*>("lookInLabel"), 0, 0, 1, 5);
    
    QSize BUTTON_SIZE = QSize(43, 33);
    QPushButton* button = (QPushButton*) findChild<QWidget*>("backButton");
    button->setIcon(QIcon());
    button->setFixedSize(BUTTON_SIZE);
    subLayout->addWidget(button, 1, 0, 1, 1);
    
    button = (QPushButton*) findChild<QWidget*>("forwardButton");
    button->setIcon(QIcon());
    button->setFixedSize(BUTTON_SIZE);
    subLayout->addWidget(button, 1, 1, 1, 1);
    
    button = (QPushButton*) findChild<QWidget*>("toParentButton");
    button->setIcon(QIcon());
    button->setFixedSize(BUTTON_SIZE);
    subLayout->addWidget(button, 1, 2, 1, 1);
    
    gridLayout->addLayout(subLayout, 0, 0, 1, 1);

    // hide unused embedded widgets in QFileDialog
    QWidget* widget = findChild<QWidget*>("lookInCombo");
    widget->hide();
    
    widget = findChild<QWidget*>("newFolderButton");
    widget->hide();

    widget = findChild<QWidget*>("listModeButton");
    widget->hide();

    widget = findChild<QWidget*>("detailModeButton");
    widget->hide();

    widget = findChild<QWidget*>("fileNameEdit");
    widget->hide();

    widget = findChild<QWidget*>("fileTypeCombo");
    widget->hide();

    widget = findChild<QWidget*>("fileTypeLabel");
    widget->hide();

    widget = findChild<QWidget*>("fileNameLabel");
    widget->hide();

    widget = findChild<QWidget*>("buttonBox");
    widget->hide();

    QSplitter* splitter = findChild<QSplitter*>("splitter");
    splitter->setHandleWidth(0);

    // remove blue outline on Mac
    widget = findChild<QWidget*>("sidebar");
    widget->setAttribute(Qt::WA_MacShowFocusRect, false);

    widget = findChild<QWidget*>("treeView");
    widget->setAttribute(Qt::WA_MacShowFocusRect, false);
    
    QFile styleSheet(PathUtils::resourcesPath() + "styles/import_dialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        QDir::setCurrent(PathUtils::resourcesPath());
        setStyleSheet(styleSheet.readAll());
    }

}

void VoxelImportDialog::setImportTypes() {
    QFile config(PathUtils::resourcesPath() + "config/config.json");
    config.open(QFile::ReadOnly | QFile::Text);
    QJsonDocument document = QJsonDocument::fromJson(config.readAll());
    if (!document.isNull() && !document.isEmpty()) {

        QString importFormatsInfo;
        QString importFormatsFilterList;
        QHash<QString, QString> iconsMap;

        QJsonObject configObject = document.object();
        if (!configObject.isEmpty()) {
            QJsonArray fileFormats = configObject["importFormats"].toArray();
            int formatsCounter = 0;
            foreach (const QJsonValue& fileFormat, fileFormats) {
                QJsonObject fileFormatObject = fileFormat.toObject();

                QString ext(fileFormatObject["extension"].toString());
                QString icon(fileFormatObject.value("icon").toString());

                if (formatsCounter > 0) {
                    importFormatsInfo.append(",");
                }

                // set ' or' on last import type text
                if (formatsCounter == fileFormats.count() - 1) {
                    importFormatsInfo.append(" or");
                }

                importFormatsFilterList.append(QString("*.%1 ").arg(ext));
                importFormatsInfo.append(" .").append(ext);
                iconsMap[ext] = icon;
                formatsCounter++;
            }
        }

        // set custom file icons
        setIconProvider(new HiFiIconProvider(iconsMap));
        setNameFilter(importFormatsFilterList);

        setLabelText(QFileDialog::LookIn, QString(IMPORT_INFO).arg(importFormatsInfo));
    }
}
