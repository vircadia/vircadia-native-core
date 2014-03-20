//
//  ModelBrowser.h
//  hifi
//
//  Created by Clement on 3/17/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__ModelBrowser__
#define __hifi__ModelBrowser__

#include <FileDownloader.h>

#include <QDialog>
#include <QListView>

static const QString S3_URL = "http://highfidelity-public.s3-us-west-1.amazonaws.com";
static const QString HEAD_MODELS_LOCATION = "models/heads/";
static const QString SKELETON_MODELS_LOCATION = "models/skeletons/";

enum ModelType {
    Head,
    Skeleton
};

class ModelBrowser : public QWidget {
    Q_OBJECT
    
public:
    ModelBrowser(ModelType modelType, QWidget* parent = NULL);
    ~ModelBrowser();
    
signals:
    void selected(QString);
    
public slots:
    void browse();
    
private slots:
    void downloadFinished();
    
private:
    ModelType _type;
    FileDownloader* _downloader;
    QHash<QString, QString> _models;
    
    bool parseXML(QByteArray xmlFile);
};



class ListView : public QListView {
    Q_OBJECT
public:
    ListView(QWidget* parent) : QListView(parent) {}
    public slots:
    void keyboardSearch(const QString& text) {
        QAbstractItemView::keyboardSearch(text);
    }
};

#endif /* defined(__hifi__ModelBrowser__) */
