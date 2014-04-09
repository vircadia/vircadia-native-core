//
//  ImportDialog.h
//  interface/src/ui
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__ImportDialog__
#define __hifi__ImportDialog__

#include "InterfaceConfig.h"

#include <QFileDialog>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QFileIconProvider>
#include <QHash>

#include <SharedUtil.h>

class HiFiIconProvider : public QFileIconProvider {
public:
    HiFiIconProvider(const QHash<QString, QString> map) { iconsMap = map; };
    virtual QIcon icon(IconType type) const;
    virtual QIcon icon(const QFileInfo &info) const;
    virtual QString type(const QFileInfo &info) const;
    QHash<QString, QString> iconsMap;
};

enum dialogMode {
    importMode,
    loadingMode,
    placeMode
};

class ImportDialog : public QFileDialog {
    Q_OBJECT
    
public:
    ImportDialog(QWidget* parent = NULL);
    void reset();
    
    QString getCurrentFile() const { return _currentFile; }
    dialogMode getMode() const { return _mode; }
    void setMode(dialogMode mode);
    
signals:
    void canceled();
    
public slots:
    void setProgressBarValue(int value);

private slots:
    void accept();
    void saveCurrentFile(QString filename);

private:
    QString _currentFile;
    QProgressBar _progressBar;
    QPushButton _importButton;
    QPushButton _cancelButton;
    dialogMode _mode;
        
    void setLayout();
    void setImportTypes();
};

#endif /* defined(__hifi__ImportDialog__) */
