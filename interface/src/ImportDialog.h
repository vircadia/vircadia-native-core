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
#include <QFileIconProvider>
#include <QHash>

#include <SharedUtil.h>

class HiFiIconProvider : public QFileIconProvider {
public:
    HiFiIconProvider(const QHash<QString, QString> map) { iconsMap = map; };
    virtual QIcon icon(IconType type) const;
    virtual QIcon icon(const QFileInfo &info) const;
    QHash<QString, QString> iconsMap;
};

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
    
    void setLayout();
    void setImportTypes();
};

#endif /* defined(__hifi__ImportDialog__) */
