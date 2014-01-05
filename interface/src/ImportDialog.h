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
    ImportDialog(QWidget* parent = NULL);
    ~ImportDialog();

    void init();
    void reset();

    QString getCurrentFile() const { return _currentFile; }

signals:
    void accepted();

public slots:
    int  exec();
    void import();
    void accept();
    void reject();

private slots:
    void saveCurrentFile(QString);

private:
    QString      _currentFile;
    QPushButton  _importButton;
};

#endif /* defined(__hifi__ImportDialog__) */
