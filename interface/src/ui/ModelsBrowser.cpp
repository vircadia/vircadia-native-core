//
//  ModelsBrowser.cpp
//  hifi
//
//  Created by Clement on 3/17/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QUrl>
#include <QXmlStreamReader>
#include <QEventLoop>
#include <QMessageBox>
#include <QGridLayout>
#include <QDialog>
#include <QStringListModel>
#include <QDialogButtonBox>
#include <QHeaderView>

#include <Application.h>

#include "ModelsBrowser.h"

static const QString S3_URL = "http://highfidelity-public.s3-us-west-1.amazonaws.com";
static const QString PUBLIC_URL = "http://public.highfidelity.io";
static const QString HEAD_MODELS_LOCATION = "models/heads";
static const QString SKELETON_MODELS_LOCATION = "models/skeletons/";

static const QString PREFIX_PARAMETER_NAME = "prefix";
static const QString MARKER_PARAMETER_NAME = "marker";
static const QString IS_TRUNCATED_NAME = "IsTruncated";
static const QString CONTAINER_NAME = "Contents";
static const QString KEY_NAME = "Key";
static const QString LOADING_MSG = "Loading...";

enum ModelMetaData {
    Name = 0,
    Creator,
    UploadeDate,
    Type,
    Gender,
    
    ModelMetaDataCount
};
static const QString propertiesNames[ModelMetaDataCount] = {
    "Name",
    "Creator",
    "Upload Date",
    "Type",
    "Gender"
};

ModelsBrowser::ModelsBrowser(ModelType modelsType, QWidget* parent) :
    QWidget(parent),
    _handler(new ModelHandler(modelsType))
{
    // Connect handler
    _handler->connect(this, SIGNAL(startDownloading()), SLOT(download()));
    _handler->connect(this, SIGNAL(startUpdating()), SLOT(update()));
    _handler->connect(this, SIGNAL(destroyed()), SLOT(exit()));
    
    // Setup and launch update thread
    QThread* thread = new QThread();
    thread->connect(_handler, SIGNAL(destroyed()), SLOT(quit()));
    thread->connect(thread, SIGNAL(finished()), SLOT(deleteLater()));
    _handler->moveToThread(thread);
    thread->start();
    emit startDownloading();
    
    // Initialize the view
    _view.setEditTriggers(QAbstractItemView::NoEditTriggers);
    _view.setRootIsDecorated(false);
    _view.setModel(_handler->getModel());
}

ModelsBrowser::~ModelsBrowser() {
}

void ModelsBrowser::applyFilter(const QString &filter) {
    QStringList filters = filter.split(" ");
    
    // Try and match every filter with each rows
    for (int i = 0; i < _handler->getModel()->rowCount(); ++i) {
        bool match = false;
        for (int k = 0; k < filters.count(); ++k) {
            match = false;
            for (int j = 0; j < ModelMetaDataCount; ++j) {
                if (_handler->getModel()->item(i, j)->text().contains(filters.at(k))) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                break;
            }
        }
        
        // Hid the row if it doesn't match (Make sure it's not it it does)
        if (match) {
            _view.setRowHidden(i, QModelIndex(), false);
        } else {
            _view.setRowHidden(i, QModelIndex(), true);
        }
    }
}

void ModelsBrowser::browse() {
    QDialog dialog;
    dialog.setWindowTitle("Browse models");
    
    QGridLayout* layout = new QGridLayout(&dialog);
    dialog.setLayout(layout);
    
    QLineEdit* searchBar = new QLineEdit(&dialog);
    layout->addWidget(searchBar, 0, 0);
    
    layout->addWidget(&_view, 1, 0);
    dialog.connect(&_view, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(accept()));
    connect(searchBar, SIGNAL(textChanged(const QString&)), SLOT(applyFilter(const QString&)));
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons, 2, 0);
    dialog.connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    dialog.connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    
    if (dialog.exec() == QDialog::Accepted) {
        QVariant selectedFile = _handler->getModel()->data(_view.currentIndex(), Qt::UserRole);
        if (selectedFile.isValid()) {
            emit selected(selectedFile.toString());
        }
    }
    
    // So that we don't have to reconstruct the view
    _view.setParent(NULL);
}


ModelHandler::ModelHandler(ModelType modelsType, QWidget* parent) :
    QObject(parent),
    _initiateExit(false),
    _type(modelsType)
{
    connect(&_downloader, SIGNAL(done(QNetworkReply::NetworkError)), SLOT(downloadFinished()));

    // set headers data
    QStringList headerData;
    for (int i = 0; i < ModelMetaDataCount; ++i) {
        headerData << propertiesNames[i];
    }
    _model.setHorizontalHeaderLabels(headerData);
}

void ModelHandler::download() {
    // Query models list
    queryNewFiles();
    
    QMutexLocker lockerModel(&_modelMutex);
    if (_initiateExit) {
        return;
    }
    // Show loading message
    QStandardItem* loadingItem = new QStandardItem(LOADING_MSG);
    loadingItem->setEnabled(false);
    _model.appendRow(loadingItem);
}

void ModelHandler::update() {
    // Will be implemented in my next PR
}

void ModelHandler::exit() {
    QMutexLocker lockerDownload(&_downloadMutex);
    QMutexLocker lockerModel(&_modelMutex);
    _initiateExit = true;
    
    // Disconnect everything
    _downloader.disconnect();
    disconnect();
    thread()->disconnect();
    
    // Make sure the thread will exit correctly
    thread()->connect(this, SIGNAL(destroyed()), SLOT(quit()));
    thread()->connect(thread(), SIGNAL(finished()), SLOT(deleteLater()));
    deleteLater();
}

void ModelHandler::downloadFinished() {
    QMutexLocker lockerDownload(&_downloadMutex);
    if (_initiateExit) {
        return;
    }
    parseXML(_downloader.getData());
}

void ModelHandler::queryNewFiles(QString marker) {
    QMutexLocker lockerDownload(&_downloadMutex);
    if (_initiateExit) {
        return;
    }
    
    // Build query
    QUrl url(S3_URL);
    QUrlQuery query;
    if (_type == Head) {
        query.addQueryItem(PREFIX_PARAMETER_NAME, HEAD_MODELS_LOCATION);
    } else if (_type == Skeleton) {
        query.addQueryItem(PREFIX_PARAMETER_NAME, SKELETON_MODELS_LOCATION);
    }
    
    if (!marker.isEmpty()) {
        query.addQueryItem(MARKER_PARAMETER_NAME, marker);
    }
    
    // Download
    url.setQuery(query);
    _downloader.download(url);
}

bool ModelHandler::parseXML(QByteArray xmlFile) {
    QMutexLocker lockerModel(&_modelMutex);
    if (_initiateExit) {
        return false;
    }
    
    QXmlStreamReader xml(xmlFile);
    QRegExp rx(".*fst");
    bool truncated = false;
    QString lastKey;
    
    // Remove loading indication
    int oldLastRow = _model.rowCount() - 1;
    delete _model.takeRow(oldLastRow).first();
    
    // Read xml until the end or an error is detected
    while(!xml.atEnd() && !xml.hasError()) {
        if (_initiateExit) {
            return false;
        }
        
        if(xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == IS_TRUNCATED_NAME) {
            while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == IS_TRUNCATED_NAME)) {
                // Let's check if there is more
                xml.readNext();
                if (xml.text().toString() == "True") {
                    truncated = true;
                }
            }
        }

        if(xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == CONTAINER_NAME) {
            while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == CONTAINER_NAME)) {
                // If a file is find, process it
                if(xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == KEY_NAME) {
                    xml.readNext();
                    lastKey = xml.text().toString();
                    if (rx.exactMatch(xml.text().toString())) {
                        // Add the found file to the list
                        QList<QStandardItem*> model;
                        model << new QStandardItem(QFileInfo(xml.text().toString()).baseName());
                        model.first()->setData(PUBLIC_URL + "/" + xml.text().toString(), Qt::UserRole);
                        
                        // Rand properties for now (Will be taken out in the next PR)
                        static QString creator[] = {"Ryan", "Philip", "Andzrej"};
                        static QString type[] = {"human", "beast", "pet", "elfe"};
                        static QString gender[] = {"male", "female", "none"};
                        model << new QStandardItem(creator[randIntInRange(0, 2)]);
                        model << new QStandardItem(QDate(randIntInRange(2013, 2014),
                                                         randIntInRange(1, 12),
                                                         randIntInRange(1, 30)).toString());
                        model << new QStandardItem(type[randIntInRange(0, 3)]);
                        model << new QStandardItem(gender[randIntInRange(0, 2)]);
                        ////////////////////////////////////////////////////////////
                        
                        _model.appendRow(model);
                    }
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }
    
    static const QString ERROR_MSG = "Error loading files";
    // Error handling
    if(xml.hasError()) {
        _model.clear();
        QStandardItem* errorItem = new QStandardItem(ERROR_MSG);
        errorItem->setEnabled(false);
        _model.appendRow(errorItem);

        return false;
    }
    
    // If we didn't all the files, download the next ones
    if (truncated) {
        // Indicate more files are being loaded
        QStandardItem* loadingItem = new QStandardItem(LOADING_MSG);
        loadingItem->setEnabled(false);
        _model.appendRow(loadingItem);
        
        // query those files
        queryNewFiles(lastKey);
    }
    
    return true;
}