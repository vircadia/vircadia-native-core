//
//  ImportDialog.cpp
//  hifi
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#include "ImportDialog.h"
#include "Application.h"

#include <QStandardPaths>
#include <QGridLayout>
#include <QMouseEvent>

const QString WINDOW_NAME                         = QObject::tr("Import Voxels");
const QString IMPORT_BUTTON_NAME                  = QObject::tr("Import");
const QString IMPORT_FILE_TYPES                   = QObject::tr("Sparse Voxel Octree Files, "
                                                                "Square PNG, "
                                                                "Schematic Files "
                                                                "(*.svo *.png *.schematic)");

const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

ImportDialog::ImportDialog(QWidget *parent)
    : QFileDialog(parent, WINDOW_NAME, DESKTOP_LOCATION, IMPORT_FILE_TYPES),
      _importButton      (IMPORT_BUTTON_NAME, this) {
    setOption(QFileDialog::DontUseNativeDialog, true);
    setFileMode(QFileDialog::ExistingFile);
    setViewMode(QFileDialog::Detail);

    QGridLayout* gridLayout = (QGridLayout*) layout();
    gridLayout->addWidget(&_importButton      , 2, 2);
    gridLayout->setColumnStretch(3, 1);

    connect(&_importButton, SIGNAL(pressed()), SLOT(import()));

    connect(this, SIGNAL(currentChanged(QString)), SLOT(saveCurrentFile(QString)));
}

ImportDialog::~ImportDialog() {
}

void ImportDialog::init() {
    VoxelSystem* voxelSystem = Application::getInstance()->getSharedVoxelSystem();
    connect(voxelSystem, SIGNAL(importSize(float,float,float)), SLOT(setGLCamera(float,float,float)));
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
