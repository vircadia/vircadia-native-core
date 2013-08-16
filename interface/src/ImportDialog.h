//
//  ImportDialog.h
//  hifi
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__ImportDialog__
#define __hifi__ImportDialog__

#include <VoxelSystem.h>

#include <QFileDialog>
#include <QPushButton>
#include <QCheckBox>
#include <QProgressBar>
#include <QGLWidget>
#include <QTimer>

class ImportDialog : public QFileDialog {
    Q_OBJECT
public:
    ImportDialog(QWidget* parent = NULL, VoxelSystem* voxelSystem = NULL);

    bool getWantPreview() const { return _previewBox.isChecked(); }
    void reset();

signals:
    void previewActivated(QString);

public slots:
    int  exec();
    void setGLCamera(float x, float y, float z);

private slots:
    void preview(bool preview);
    void saveCurrentFile(QString);
    void timer();

private:
    QString      _currentFile;
    QCheckBox    _clipboardImportBox;
    QCheckBox    _previewBox;
    QProgressBar _previewBar;
    QGLWidget*   _glPreview;
    QTimer       _glTimer;
};

#endif /* defined(__hifi__ImportDialog__) */
