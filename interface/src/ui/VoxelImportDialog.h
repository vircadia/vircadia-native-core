//
//  VoxelImportDialog.h
//  interface/src/ui
//
//  Created by Clement Brisset on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelImportDialog_h
#define hifi_VoxelImportDialog_h

#include "InterfaceConfig.h"

#include <QFileDialog>
#include <QPushButton>
#include <QProgressBar>
#include <QLabel>
#include <QFileIconProvider>
#include <QHash>

#include <SharedUtil.h>

#include "voxels/VoxelImporter.h"

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
    finishedMode
};

class VoxelImportDialog : public QFileDialog {
    Q_OBJECT
    
public:
    VoxelImportDialog(QWidget* parent = NULL);
    
    QString getCurrentFile() const { return _currentFile; }
    dialogMode getMode() const { return _mode; }

    void setMode(dialogMode mode);
    void reset();
    bool prompt();
    void loadSettings(QSettings* settings);
    void saveSettings(QSettings* settings);
    
signals:
    void canceled();

private slots:
    void setProgressBarValue(int value);
    void accept();
    void cancel();
    void saveCurrentFile(QString filename);
    void afterImport();

private:
    QPushButton _cancelButton;
    QString _currentFile;
    QPushButton _importButton;
    VoxelImporter* _importer;
    dialogMode _mode;
    QProgressBar _progressBar;
    bool _didImport;
        
    void setLayout();
    void setImportTypes();
};

#endif // hifi_VoxelImportDialog_h
