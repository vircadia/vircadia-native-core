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

static const int DOWNLOAD_TIMEOUT = 1000;
static const QString CONTAINER_NAME = "Contents";
static const QString KEY_NAME = "Key";

ModelBrowser::ModelBrowser(QWidget* parent) :
    QWidget(parent),
    _downloader(QUrl(S3_URL))
{
}

QString ModelBrowser::browse(ModelType modelType) {
    _models.clear();
    if (!parseXML(modelType)) {
        return QString();
    }
    
    
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
        return QString();
    }
    
    QString selectedKey = model->data(listView->currentIndex(), Qt::DisplayRole).toString();
    return _models[selectedKey];
}

void ModelBrowser::browseHead() {
    QString model = browse(Head);
    emit selectedHead(model);
}

void ModelBrowser::browseSkeleton() {
    QString model = browse(Skeleton);
    emit selectedSkeleton(model);
}

bool ModelBrowser::parseXML(ModelType modelType) {
    _downloader.waitForFile(DOWNLOAD_TIMEOUT);
    QString location;
    switch (modelType) {
        case Head:
            location = HEAD_MODELS_LOCATION;
            break;
        case Skeleton:
            location = SKELETON_MODELS_LOCATION;
            break;
        default:
            return false;
    }
    
    QXmlStreamReader xml(_downloader.getData());
    QRegExp rx(location + "[^/]*fst");
    
    // Read xml until the end or an error is detected
    while(!xml.atEnd() && !xml.hasError()) {
        if(xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == CONTAINER_NAME) {
            while(!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == CONTAINER_NAME)) {
                // If a file is find, process it
                if(xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == KEY_NAME) {
                    xml.readNext();
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
    return true;
}