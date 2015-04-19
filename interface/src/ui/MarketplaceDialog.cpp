//
//  AddressBarDialog.cpp
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Application.h"
#include "MarketplaceDialog.h"


#include <QWebEnginePage>

#include "DependencyManager.h"

QML_DIALOG_DEF(MarketplaceDialog)


MarketplaceDialog::MarketplaceDialog(QQuickItem *parent) : OffscreenQmlDialog(parent) {
    this->
}

bool MarketplaceDialog::navigationRequested(const QString & url) {
    qDebug() << url;
    if (Application::getInstance()->canAcceptURL(url)) {
        qDebug() << "Trying to send url to the app";
        if (Application::getInstance()->acceptURL(url)) {
            qDebug() << "Sent url to the app";
            return false; // we handled it, so QWebPage doesn't need to handle it
        }
    }
    return true;
}

//https://metaverse.highfidelity.com/marketplace

//
//bool DataWebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, QWebPage::NavigationType type) {
//    QString urlString = request.url().toString();
//    if (Application::getInstance()->canAcceptURL(urlString)) {
//        if (Application::getInstance()->acceptURL(urlString)) {
//            return false; // we handled it, so QWebPage doesn't need to handle it
//        }
//    }
//    return true;
//}