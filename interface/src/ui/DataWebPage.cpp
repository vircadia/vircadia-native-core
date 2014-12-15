//
//  DataWebPage.cpp
//  interface/src/ui
//
//  Created by Stephen Birarda on 2014-09-22.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <qnetworkrequest.h>

#include <AddressManager.h>
#include <OAuthNetworkAccessManager.h>

#include "DataWebPage.h"

DataWebPage::DataWebPage(QObject* parent) :
    QWebPage(parent)
{
    // use an OAuthNetworkAccessManager instead of regular QNetworkAccessManager so our requests are authed
    setNetworkAccessManager(OAuthNetworkAccessManager::getInstance());
    
    // give the page an empty stylesheet
    settings()->setUserStyleSheetUrl(QUrl());
}

void DataWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID) {
    qDebug() << "JS console message at line" << lineNumber << "from" << sourceID << "-" << message;
}

bool DataWebPage::acceptNavigationRequest(QWebFrame* frame, const QNetworkRequest& request, QWebPage::NavigationType type) {
    
    if (!request.url().toString().startsWith(HIFI_URL_SCHEME)) {
        return true;
    } else {
        // this is a hifi URL - have the AddressManager handle it
        QMetaObject::invokeMethod(DependencyManager::get<AddressManager>(), "handleLookupString",
                                  Qt::AutoConnection, Q_ARG(const QString&, request.url().toString()));
        return false;
    }
}