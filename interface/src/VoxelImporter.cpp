//
//  VoxelImporter.cpp
//  hifi
//
//  Created by Clement Brisset on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#include "VoxelImporter.h"

#include <QObject>
#include <QStandardPaths>
#include <QGridLayout>

const QString WINDOW_NAME                     = QObject::tr("Import Voxels");
const QString PREVIEW_CHECKBOX_STRING         = QObject::tr("Load preview");
const QString IMPORT_BUTTON_NAME              = QObject::tr("Import");
const QString IMPORT_TO_CLIPBOARD_BUTTON_NAME = QObject::tr("Import into clipboard");
const QString IMPORT_FILE_TYPES               = QObject::tr("Sparse Voxel Octree Files, "
                                                            "Square PNG, "
                                                            "Schematic Files "
                                                            "(*.svo *.png *.schematic)");

const QString DESKTOP_LOCATION = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);

VoxelImporter::VoxelImporter(QWidget* parent)
    : QFileDialog(parent, WINDOW_NAME, DESKTOP_LOCATION, IMPORT_FILE_TYPES),
      _importButton         (IMPORT_BUTTON_NAME             , this),
      _clipboardImportButton(IMPORT_TO_CLIPBOARD_BUTTON_NAME, this),
      _previewBox           (PREVIEW_CHECKBOX_STRING        , this),
      _previewBar           (this),
      _glPreview            (this) {

    setOption(QFileDialog::DontUseNativeDialog, true);
    setFileMode(QFileDialog::ExistingFile);
    setViewMode(QFileDialog::Detail);
    _previewBar.setVisible(false);
    _glPreview.setVisible(false);

    QGridLayout* gridLayout = (QGridLayout*) layout();
    gridLayout->addWidget(&_importButton         , 2, 2);
    gridLayout->addWidget(&_clipboardImportButton, 2, 3);
    gridLayout->addWidget(&_previewBox           , 3, 3);
    gridLayout->addWidget(&_previewBar           , 0, 3);
    gridLayout->addWidget(&_glPreview            , 1, 3);


    connect(&_importButton         , SIGNAL(pressed()), this, SLOT(importVoxels()));
    connect(&_clipboardImportButton, SIGNAL(pressed()), this, SLOT(importVoxelsToClipboard()));
    connect(&_previewBox, SIGNAL(toggled(bool)), &_previewBar, SLOT(setVisible(bool)));
    connect(&_previewBox, SIGNAL(toggled(bool)), &_glPreview , SLOT(setVisible(bool)));
}

int VoxelImporter::exec() {
    _previewBox.setChecked(false);
    _previewBar.setVisible(false);
    _glPreview.setVisible(false);

    return exec();
}


void VoxelImporter::importVoxels() {
    accept();
    qDebug("Congratulation, you imported a file.\n");
}

void VoxelImporter::importVoxelsToClipboard() {
    accept();
    qDebug("Congratulation, you imported a file to the clipboard.\n");

}

void VoxelImporter::preview(bool) {

}
