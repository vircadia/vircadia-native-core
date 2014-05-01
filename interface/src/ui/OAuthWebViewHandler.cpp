//
//  OAuthWebViewHandler.cpp
//  interface/src/ui
//
//  Created by Stephen Birarda on 2014-05-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtWebkitWidgets/QWebView>

#include "Application.h"

#include "OAuthWebviewHandler.h"

OAuthWebViewHandler& OAuthWebViewHandler::getInstance() {
    static OAuthWebViewHandler sharedInstance;
    return sharedInstance;
}

OAuthWebViewHandler::OAuthWebViewHandler() :
    _activeWebView(NULL)
{
    
}

void OAuthWebViewHandler::displayWebviewForAuthorizationURL(const QUrl& authorizationURL) {
    if (!_activeWebView) {
        _activeWebView = new QWebView();
        _activeWebView->setWindowFlags(Qt::WindowStaysOnTopHint);
        _activeWebView->load(authorizationURL);
        _activeWebView->show();
        
        connect(_activeWebView->page()->networkAccessManager(), &QNetworkAccessManager::sslErrors,
                this, &OAuthWebViewHandler::handleSSLErrors);
    }
}

void OAuthWebViewHandler::handleSSLErrors(QNetworkReply* networkReply, const QList<QSslError>& errorList) {
    qDebug() << "SSL Errors:" << errorList;
}
