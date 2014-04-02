//
//  ModelsBrowser.h
//  hifi
//
//  Created by Clement on 3/17/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ModelsBrowser__
#define __hifi__ModelsBrowser__

#include <QAbstractTableModel>
#include <QStandardItem>
#include <QTreeView>
#include <QVector>
#include <QReadWriteLock>

#include "FileDownloader.h"

enum ModelType {
    Head,
    Skeleton
};

class ModelHandler : public QObject {
    Q_OBJECT
public:
    ModelHandler(ModelType modelsType, QWidget* parent = NULL);
    
    void lockModel() { _lock.lockForRead(); }
    QStandardItemModel* getModel() { return &_model; }
    void unlockModel() { _lock.unlock(); }
    
signals:
    void doneDownloading();
    void doneUpdating();
    
public slots:
    void download();
    void update();
    void exit();
    
private slots:
    void downloadFinished();
    
private:
    bool _initiateExit;
    ModelType _type;
    FileDownloader _downloader;
    QReadWriteLock _lock;
    QStandardItemModel _model;
    
    void queryNewFiles(QString marker = QString());
    bool parseXML(QByteArray xmlFile);
};


class ModelsBrowser : public QWidget {
    Q_OBJECT
public:
    
    ModelsBrowser(ModelType modelsType, QWidget* parent = NULL);
    
signals:
    void startDownloading();
    void startUpdating();
    void selected(QString filename);
    
public slots:
    void browse();
    
private slots:
    void applyFilter(const QString& filter);
    
private:
    ModelHandler* _handler;
    QTreeView _view;
};

#endif /* defined(__hifi__ModelBrowser__) */
