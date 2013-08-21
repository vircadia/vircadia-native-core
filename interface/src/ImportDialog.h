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

class GLWidget;

class ImportDialog : public QFileDialog {
    Q_OBJECT
public:
    ImportDialog(QWidget* parent = NULL, VoxelSystem* voxelSystem = NULL);
    ~ImportDialog();

    bool getWantPreview() const { return _previewBox.isChecked(); }
    QString getCurrentFile() const { return _currentFile; }
    bool getImportIntoClipboard() const { return _clipboardImportBox.isChecked(); }

    void reset();

signals:
    void previewToggled(bool);
    void accepted();

public slots:
    int  exec();
    void setGLCamera(float x, float y, float z);
    void import();
    void accept();
    void reject();

private slots:
    void preview(bool preview);
    void saveCurrentFile(QString);
    void timer();

private:
    QString      _currentFile;
    QPushButton  _importButton;
    QCheckBox    _clipboardImportBox;
    QCheckBox    _previewBox;
    QProgressBar _previewBar;
    GLWidget*    _glPreview;
    QTimer       _glTimer;
};

#endif /* defined(__hifi__ImportDialog__) */
