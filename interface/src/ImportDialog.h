//
//  ImportDialog.h
//  hifi
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__ImportDialog__
#define __hifi__ImportDialog__

#include <QFileDialog>
#include <QPushButton>
#include <QLabel>
#include <SharedUtil.h>

class ImportDialog : public QFileDialog {
    Q_OBJECT
    
public:
    ImportDialog(QWidget* parent = NULL);
    ~ImportDialog();

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
    QString _currentFile;
    QPushButton _importButton;
    QPushButton _cancelButton;
    QLabel _importLabel;
    QLabel _infoLabel;
    
    void setLayout();
};

#endif /* defined(__hifi__ImportDialog__) */
