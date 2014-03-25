//
//  ModelBrowser.cpp
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

#include <Application.h>

#include "ModelBrowser.h"

static const QString PREFIX_PARAMETER_NAME = "prefix";
static const QString MARKER_PARAMETER_NAME = "marker";
static const QString IS_TRUNCATED_NAME = "IsTruncated";
static const QString CONTAINER_NAME = "Contents";
static const QString KEY_NAME = "Key";

ModelBrowser::ModelBrowser(ModelType modelType, QWidget* parent) : QWidget(parent), _type(modelType) {
    QUrl url(S3_URL);
    QUrlQuery query;
    
    if (_type == Head) {
        query.addQueryItem(PREFIX_PARAMETER_NAME, HEAD_MODELS_LOCATION);
    } else if (_type == Skeleton) {
        query.addQueryItem(PREFIX_PARAMETER_NAME, SKELETON_MODELS_LOCATION);
    }
    url.setQuery(query);
    
    _downloader = new FileDownloader(url);
    connect(_downloader, SIGNAL(done(QNetworkReply::NetworkError)), SLOT(downloadFinished()));
}

ModelBrowser::~ModelBrowser() {
    delete _downloader;
}

void ModelBrowser::downloadFinished() {
    parseXML(_downloader->getData());
}

void ModelBrowser::browse() {
    QDialog dialog(this);
    dialog.setWindowTitle("Browse models");
    
    QGridLayout* layout = new QGridLayout(&dialog);
    dialog.setLayout(layout);
    
    QLineEdit* searchBar = new QLineEdit(&dialog);
    layout->addWidget(searchBar, 0, 0);
    
    ListView* listView = new ListView(&dialog);
    listView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(listView, 1, 0);
    listView->connect(searchBar, SIGNAL(textChanged(const QString&)), SLOT(keyboardSearch(const QString&)));
    dialog.connect(listView, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(accept()));
    
    QStringListModel* model = new QStringListModel(_models.keys(), listView);
    model->sort(0);
    listView->setModel(model);
    
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons, 2, 0);
    dialog.connect(buttons, SIGNAL(accepted()), SLOT(accept()));
    dialog.connect(buttons, SIGNAL(rejected()), SLOT(reject()));
    
    if (dialog.exec() == QDialog::Rejected) {
        return;
    }
    
    QString selectedKey = model->data(listView->currentIndex(), Qt::DisplayRole).toString();
    
    emit selected(_models[selectedKey]);
}

bool ModelBrowser::parseXML(QByteArray xmlFile) {
    QXmlStreamReader xml(xmlFile);
    QRegExp rx(".*fst");
    bool truncated = false;
    QString lastKey;
    
    // Read xml until the end or an error is detected
    while(!xml.atEnd() && !xml.hasError()) {
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
                        _models.insert(QFileInfo(xml.text().toString()).baseName(),
                                       S3_URL + "/" + xml.text().toString());
                    }
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }
    
    // Error handling
    if(xml.hasError()) {
        _models.clear();
        QMessageBox::critical(this,
                              "ModelBrowser::ModelBrowser()",
                              xml.errorString(),
                              QMessageBox::Ok);
        return false;
    }
    
    // If we didn't all the files, download the next ones
    if (truncated) {
        QUrl url(S3_URL);
        QUrlQuery query;
        
        if (_type == Head) {
            query.addQueryItem(PREFIX_PARAMETER_NAME, HEAD_MODELS_LOCATION);
        } else if (_type == Skeleton) {
            query.addQueryItem(PREFIX_PARAMETER_NAME, SKELETON_MODELS_LOCATION);
        }
        query.addQueryItem(MARKER_PARAMETER_NAME, lastKey);
        url.setQuery(query);
        
        delete _downloader;
        _downloader = new FileDownloader(url);
        connect(_downloader, SIGNAL(done(QNetworkReply::NetworkError)), SLOT(downloadFinished()));
    }
    
    return true;
}