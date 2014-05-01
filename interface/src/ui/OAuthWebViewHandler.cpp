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

void OAuthWebViewHandler::addHighFidelityRootCAToSSLConfig() {
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    
    // add the High Fidelity root CA to the list of trusted CA certificates
    QByteArray highFidelityCACertificate(reinterpret_cast<char*>(DTLSSession::highFidelityCADatum()->data),
                                         DTLSSession::highFidelityCADatum()->size);
    sslConfig.setCaCertificates(sslConfig.caCertificates() + QSslCertificate::fromData(highFidelityCACertificate));
    
    // set the modified configuration
    QSslConfiguration::setDefaultConfiguration(sslConfig);
}

const int WEB_VIEW_REDISPLAY_ELAPSED_MSECS = 5 * 1000;

void OAuthWebViewHandler::displayWebviewForAuthorizationURL(const QUrl& authorizationURL) {
    if (!_activeWebView && _webViewRedisplayTimer.elapsed() >= WEB_VIEW_REDISPLAY_ELAPSED_MSECS) {
        _activeWebView = new QWebView;
        
        // keep the window on top and delete it when it closes
        _activeWebView->setWindowFlags(Qt::WindowStaysOnTopHint);
        _activeWebView->setAttribute(Qt::WA_DeleteOnClose);
        
        qDebug() << "Displaying QWebView for OAuth authorization at" << authorizationURL.toString();
        _activeWebView->load(authorizationURL);
        _activeWebView->show();
        
        connect(_activeWebView->page()->networkAccessManager(), &QNetworkAccessManager::sslErrors,
                this, &OAuthWebViewHandler::handleSSLErrors);
        connect(_activeWebView, &QWebView::loadFinished, this, &OAuthWebViewHandler::handleLoadFinished);
        
        // connect to the destroyed signal so after the web view closes we can start a timer
        connect(_activeWebView, &QWebView::destroyed, this, &OAuthWebViewHandler::handleWebViewDestroyed);
    }
}

void OAuthWebViewHandler::handleSSLErrors(QNetworkReply* networkReply, const QList<QSslError>& errorList) {
    qDebug() << "SSL Errors:" << errorList;
}

void OAuthWebViewHandler::handleLoadFinished(bool success) {
    if (success && _activeWebView->url().host() == NodeList::getInstance()->getDomainHandler().getHostname()) {
        qDebug() << "OAuth authorization code passed successfully to domain-server.";
        _activeWebView->close();
    }
}

void OAuthWebViewHandler::handleWebViewDestroyed(QObject* destroyedObject) {
    _webViewRedisplayTimer.restart();
}
