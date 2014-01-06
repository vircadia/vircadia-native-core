//
//  ImportDialog.cpp
//  hifi
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#include "ImportDialog.h"

#include <QStandardPaths>
#include <QGridLayout>
#include <QSplitter>

const QString WINDOW_NAME = QObject::tr("Import Voxels");
const QString IMPORT_BUTTON_NAME = QObject::tr("Import");
const QString CANCEL_BUTTON_NAME = QObject::tr("Cancel");
const QString IMPORT_FILE_TYPES = QObject::tr("Sparse Voxel Octree Files, "
                                              "Square PNG, "
                                              "Schematic Files "
                                              "(*.svo *.png *.schematic)");
const QString INFO_LABEL_TEXT = QObject::tr("This will load selected file into Hifi and\n"
                                            "allow you to place it with Command V");

const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

ImportDialog::ImportDialog(QWidget *parent) : QFileDialog(parent, WINDOW_NAME, DESKTOP_LOCATION, IMPORT_FILE_TYPES),
_importButton(IMPORT_BUTTON_NAME, this),
_cancelButton(CANCEL_BUTTON_NAME, this),
_importLabel(IMPORT_BUTTON_NAME),
_infoLabel(INFO_LABEL_TEXT) {
    
    setOption(QFileDialog::DontUseNativeDialog, true);
    setFileMode(QFileDialog::ExistingFile);
    setViewMode(QFileDialog::Detail);

    setLayout();
    QGridLayout* gridLayout = (QGridLayout*) layout();
    gridLayout->addWidget(&_importLabel, 0, 0);
    gridLayout->addWidget(&_infoLabel, 2, 0);
    gridLayout->addWidget(&_cancelButton, 2, 1);
    gridLayout->addWidget(&_importButton, 2, 2);
    gridLayout->setColumnStretch(3, 1);

    connect(&_importButton, SIGNAL(pressed()), SLOT(import()));
    connect(this, SIGNAL(currentChanged(QString)), SLOT(saveCurrentFile(QString)));
}

ImportDialog::~ImportDialog() {
}

void ImportDialog::import() {
    _importButton.setDisabled(true);
    emit accepted();
}

void ImportDialog::accept() {
    QFileDialog::accept();
}

void ImportDialog::reject() {
    QFileDialog::reject();
}

int ImportDialog::exec() {
    return QFileDialog::exec();
}

void ImportDialog::reset() {
    _importButton.setEnabled(true);
}

void ImportDialog::saveCurrentFile(QString filename) {
    _currentFile = filename;
}

void ImportDialog::setLayout() {
    
    // set ObjectName used in qss
    _importButton.setObjectName("importButton");
    _cancelButton.setObjectName("cancelButton");
    
    // set size policy used in
    _importButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _cancelButton.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // hide unused embeded widgets in QFileDialog
    QWidget* widget = findChild<QWidget*>("lookInLabel");
    widget->hide();
    
    widget = findChild<QWidget*>("lookInCombo");
    widget->hide();
    
    widget = findChild<QWidget*>("backButton");
    widget->hide();
    
    widget = findChild<QWidget*>("forwardButton");
    widget->hide();
    
    widget = findChild<QWidget*>("toParentButton");
    widget->hide();
    
    widget = findChild<QWidget*>("newFolderButton");
    widget->hide();
    
    widget = findChild<QWidget*>("listModeButton");
    widget->hide();
    
    widget = findChild<QWidget*>("detailModeButton");
    widget->hide();
    
    widget = findChild<QWidget*>("fileNameEdit");
    widget->hide();
    
    widget = findChild<QWidget*>("fileNameLabel");
    widget->hide();
    
    widget = findChild<QWidget*>("fileTypeCombo");
    widget->hide();
    
    widget = findChild<QWidget*>("fileTypeLabel");
    widget->hide();
    
    widget = findChild<QWidget*>("buttonBox");
    widget->hide();
    
    QSplitter *splitter = findChild<QSplitter*>("splitter");
    splitter->setHandleWidth(0);
    
    // remove blue outline on Mac
    widget = findChild<QWidget*>("sidebar");
    widget->setAttribute(Qt::WA_MacShowFocusRect, false);
    widget = findChild<QWidget*>("treeView");
    widget->setAttribute(Qt::WA_MacShowFocusRect, false);
    
    // set custom file icons
    // setIconProvider(new HiFiIconProvider());
    
    switchToResourcesParentIfRequired();
    QFile styleSheet("resources/styles/import_dialog.qss");
    if (styleSheet.open(QIODevice::ReadOnly)) {
        setStyleSheet(styleSheet.readAll());
    }
}
