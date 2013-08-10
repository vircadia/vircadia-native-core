//
//  VoxelImporter.h
//  hifi
//
//  Created by Clement Brisset on 8/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelImporter__
#define __hifi__VoxelImporter__

#include <QFileDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QGLWidget>

class VoxelImporter : public QFileDialog {
    Q_OBJECT
public:
    VoxelImporter(QWidget* parent = NULL);

public slots:
    int exec();
    void importVoxels();
    void importVoxelsToClipboard();
    void preview(bool);

private:
    QPushButton  _importButton;
    QPushButton  _clipboardImportButton;
    QCheckBox    _previewBox;
    QProgressBar _previewBar;
    QGLWidget    _glPreview;
};

#endif /* defined(__hifi__VoxelImporter__) */
