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

#include <OAuthNetworkAccessManager.h>

#include "DataWebPage.h"

DataWebPage::DataWebPage(QObject* parent) :
    QWebPage(parent)
{
    // use an OAuthNetworkAccessManager instead of regular QNetworkAccessManager so our requests are authed
    setNetworkAccessManager(OAuthNetworkAccessManager::getInstance());
    
    // have the page delegate external links so they can be captured by the Application in case they are a hifi link
    setLinkDelegationPolicy(QWebPage::DelegateExternalLinks);
    
    // give the page an empty stylesheet
    settings()->setUserStyleSheetUrl(QUrl());
}

void DataWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID) {
    qDebug() << "JS console message at line" << lineNumber << "from" << sourceID << "-" << message;
}