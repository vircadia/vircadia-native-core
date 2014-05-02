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

#include <QtWebKitWidgets/QWebView>

#include "Application.h"

#include "OAuthWebviewHandler.h"

OAuthWebViewHandler& OAuthWebViewHandler::getInstance() {
    static OAuthWebViewHandler sharedInstance;
    return sharedInstance;
}

OAuthWebViewHandler::OAuthWebViewHandler() :
    _activeWebView(NULL),
    _webViewRedisplayTimer(),
    _lastAuthorizationURL()
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
    if (!_activeWebView) {

        if (!_lastAuthorizationURL.isEmpty()) {
            if (_lastAuthorizationURL.host() == authorizationURL.host()
                && _webViewRedisplayTimer.elapsed() < WEB_VIEW_REDISPLAY_ELAPSED_MSECS) {
                // this would be re-displaying an OAuth dialog for the same auth URL inside of the redisplay ms
                // so return instead
                return;
            }
        }
        
        _lastAuthorizationURL = authorizationURL;
        
        _activeWebView = new QWebView;
        
        // keep the window on top and delete it when it closes
        _activeWebView->setWindowFlags(Qt::WindowStaysOnTopHint);
        _activeWebView->setAttribute(Qt::WA_DeleteOnClose);
        
        qDebug() << "Displaying QWebView for OAuth authorization at" << authorizationURL.toString();
        
        AccountManager& accountManager = AccountManager::getInstance();
        
        QUrl codedAuthorizationURL = authorizationURL;
        
        // check if we have an access token for this host - if so we can bypass login by adding it to the URL
        if (accountManager.getAuthURL().host() == authorizationURL.host()
            && accountManager.hasValidAccessToken()) {
            
            const QString ACCESS_TOKEN_QUERY_STRING_KEY = "access_token";
            
            QUrlQuery authQuery(codedAuthorizationURL);
            authQuery.addQueryItem(ACCESS_TOKEN_QUERY_STRING_KEY, accountManager.getAccountInfo().getAccessToken().token);
            
            codedAuthorizationURL.setQuery(authQuery);
        }
        
        _activeWebView->load(codedAuthorizationURL);
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
        
        // grab the UUID that is set as the state parameter in the auth URL
        // since that is our new session UUID
        QUrlQuery authQuery(_activeWebView->url());
        
        const QString AUTH_STATE_QUERY_KEY = "state";
        NodeList::getInstance()->setSessionUUID(QUuid(authQuery.queryItemValue(AUTH_STATE_QUERY_KEY)));
        
        _activeWebView->close();
    }
}

void OAuthWebViewHandler::handleWebViewDestroyed(QObject* destroyedObject) {
    _webViewRedisplayTimer.restart();
}
