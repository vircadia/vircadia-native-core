//
//  ModelsBrowser.h
//  interface/src/ui
//
//  Created by Clement on 3/17/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelsBrowser_h
#define hifi_ModelsBrowser_h

#include <QReadWriteLock>
#include <QStandardItemModel>
#include <QTreeView>

enum ModelType {
    HEAD_MODEL,
    SKELETON_MODEL,
    ATTACHMENT_MODEL
};

extern const char* MODEL_TYPE_NAMES[];

class ModelHandler : public QObject {
    Q_OBJECT
public:
    ModelHandler(ModelType modelsType, QWidget* parent = NULL);
    
    void lockModel() { _lock.lockForRead(); }
    QStandardItemModel* getModel() { return &_model; }
    void unlockModel() { _lock.unlock(); }
    
signals:
    void doneDownloading();
    void updated();
    
public slots:
    void download();
    void update();
    void exit();
    
private slots:
    void downloadFinished(QNetworkReply* reply);
    
private:
    bool _initiateExit;
    ModelType _type;
    QReadWriteLock _lock;
    QStandardItemModel _model;
    
    void queryNewFiles(QString marker = QString());
    bool parseXML(QByteArray xmlFile);
    bool parseHeaders(QNetworkReply* reply);
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
    void resizeView();
    void enableSearchBar();
    
private:
    ModelHandler* _handler;
    QLineEdit* _searchBar;
    QTreeView _view;
};

#endif // hifi_ModelsBrowser_h
