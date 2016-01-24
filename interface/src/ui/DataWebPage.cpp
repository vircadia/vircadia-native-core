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

#include "Application.h"
#include <AddressManager.h>
#include <OAuthNetworkAccessManager.h>

#include "DataWebPage.h"

DataWebPage::DataWebPage(QObject* parent) : QWebEnginePage(parent)
{
    // use an OAuthNetworkAccessManager instead of regular QNetworkAccessManager so our requests are authed
//    setNetworkAccessManager(OAuthNetworkAccessManager::getInstance());

    // give the page an empty stylesheet
//    settings()->setUserStyleSheetUrl(QUrl());
}

//void DataWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID) {
//    qDebug() << "JS console message at line" << lineNumber << "from" << sourceID << "-" << message;
//}

bool DataWebPage::acceptNavigationRequest(const QUrl & url, NavigationType type, bool isMainFrame) {
    // Handle hifi:// links and links to files with particular extensions
    QString urlString = url.toString();
    if (qApp->canAcceptURL(urlString)) {
        if (qApp->acceptURL(urlString)) {
            return false; // we handled it, so QWebPage doesn't need to handle it
        }
    }

    // Make hyperlinks with target="_blank" open in user's Web browser
    if (type == NavigationTypeLinkClicked && !isMainFrame) {
        qApp->openUrl(url);
        return false;  // We handled it.
    }

    return true;
}

//
//QString DataWebPage::userAgentForUrl(const QUrl& url) const {
//    return HIGH_FIDELITY_USER_AGENT;
//}
