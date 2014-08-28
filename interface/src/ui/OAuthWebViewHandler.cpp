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

#include "OAuthWebViewHandler.h"

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

const char HIGH_FIDELITY_CA[] = "-----BEGIN CERTIFICATE-----\n"
    "MIID6TCCA1KgAwIBAgIJANlfRkRD9A8bMA0GCSqGSIb3DQEBBQUAMIGqMQswCQYD\n"
    "VQQGEwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEWMBQGA1UEBxMNU2FuIEZyYW5j\n"
    "aXNjbzEbMBkGA1UEChMSSGlnaCBGaWRlbGl0eSwgSW5jMRMwEQYDVQQLEwpPcGVy\n"
    "YXRpb25zMRgwFgYDVQQDEw9oaWdoZmlkZWxpdHkuaW8xIjAgBgkqhkiG9w0BCQEW\n"
    "E29wc0BoaWdoZmlkZWxpdHkuaW8wHhcNMTQwMzI4MjIzMzM1WhcNMjQwMzI1MjIz\n"
    "MzM1WjCBqjELMAkGA1UEBhMCVVMxEzARBgNVBAgTCkNhbGlmb3JuaWExFjAUBgNV\n"
    "BAcTDVNhbiBGcmFuY2lzY28xGzAZBgNVBAoTEkhpZ2ggRmlkZWxpdHksIEluYzET\n"
    "MBEGA1UECxMKT3BlcmF0aW9uczEYMBYGA1UEAxMPaGlnaGZpZGVsaXR5LmlvMSIw\n"
    "IAYJKoZIhvcNAQkBFhNvcHNAaGlnaGZpZGVsaXR5LmlvMIGfMA0GCSqGSIb3DQEB\n"
    "AQUAA4GNADCBiQKBgQDyo1euYiPPEdnvDZnIjWrrP230qUKMSj8SWoIkbTJF2hE8\n"
    "2eP3YOgbgSGBzZ8EJBxIOuNmj9g9Eg6691hIKFqy5W0BXO38P04Gg+pVBvpHFGBi\n"
    "wpqGbfsjaUDuYmBeJRcMO0XYkLCRQG+lAQNHoFDdItWAJfC3FwtP3OCDnz8cNwID\n"
    "AQABo4IBEzCCAQ8wHQYDVR0OBBYEFCSv2kmiGg6VFMnxXzLDNP304cPAMIHfBgNV\n"
    "HSMEgdcwgdSAFCSv2kmiGg6VFMnxXzLDNP304cPAoYGwpIGtMIGqMQswCQYDVQQG\n"
    "EwJVUzETMBEGA1UECBMKQ2FsaWZvcm5pYTEWMBQGA1UEBxMNU2FuIEZyYW5jaXNj\n"
    "bzEbMBkGA1UEChMSSGlnaCBGaWRlbGl0eSwgSW5jMRMwEQYDVQQLEwpPcGVyYXRp\n"
    "b25zMRgwFgYDVQQDEw9oaWdoZmlkZWxpdHkuaW8xIjAgBgkqhkiG9w0BCQEWE29w\n"
    "c0BoaWdoZmlkZWxpdHkuaW+CCQDZX0ZEQ/QPGzAMBgNVHRMEBTADAQH/MA0GCSqG\n"
    "SIb3DQEBBQUAA4GBAEkQl3p+lH5vuoCNgyfa67nL0MsBEt+5RSBOgjwCjjASjzou\n"
    "FTv5w0he2OypgMQb8i/BYtS1lJSFqjPJcSM1Salzrm3xDOK5pOXJ7h6SQLPDVEyf\n"
    "Hy2/9d/to+99+SOUlvfzfgycgjOc+s/AV7Y+GBd7uzGxUdrN4egCZW1F6/mH\n"
    "-----END CERTIFICATE-----\n";

void OAuthWebViewHandler::addHighFidelityRootCAToSSLConfig() {
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    
    // add the High Fidelity root CA to the list of trusted CA certificates
    QByteArray highFidelityCACertificate(HIGH_FIDELITY_CA, sizeof(HIGH_FIDELITY_CA));
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
        _activeWebView->setWindowFlags(Qt::Sheet);
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
        
        connect(_activeWebView.data(), &QWebView::urlChanged, this, &OAuthWebViewHandler::handleURLChanged);
        
        _activeWebView->load(codedAuthorizationURL);
        
        connect(_activeWebView->page()->networkAccessManager(), &QNetworkAccessManager::sslErrors,
                this, &OAuthWebViewHandler::handleSSLErrors);
        connect(_activeWebView->page()->networkAccessManager(), &QNetworkAccessManager::finished,
                this, &OAuthWebViewHandler::handleReplyFinished);
        connect(_activeWebView.data(), &QWebView::loadFinished, this, &OAuthWebViewHandler::handleLoadFinished);
        
        // connect to the destroyed signal so after the web view closes we can start a timer
        connect(_activeWebView.data(), &QWebView::destroyed, this, &OAuthWebViewHandler::handleWebViewDestroyed);
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
        _activeWebView = NULL;
    }
}

void OAuthWebViewHandler::handleReplyFinished(QNetworkReply* reply) {
    if (_activeWebView && reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error loading" << reply->url() << "-" << reply->errorString();
        _activeWebView->close();
    }
}

void OAuthWebViewHandler::handleWebViewDestroyed(QObject* destroyedObject) {
    _webViewRedisplayTimer.restart();
}

void OAuthWebViewHandler::handleURLChanged(const QUrl& url) {
    // check if this is the authorization screen - if it is then we need to show the OAuthWebViewHandler
    const QString ACCESS_TOKEN_URL_REGEX_STRING = "redirect_uri=[\\w:\\/\\.]+&access_token=";
    QRegExp accessTokenRegex(ACCESS_TOKEN_URL_REGEX_STRING);
    
    if (accessTokenRegex.indexIn(url.toString()) != -1) {
        _activeWebView->show();
    } else if (url.toString() == DEFAULT_NODE_AUTH_URL.toString() + "/login") {
        // this is a login request - we're going to close the webview and signal the AccountManager that we need a login
        qDebug() << "data-server replied with login request. Signalling that login is required to proceed with OAuth.";
        _activeWebView->close();
        AccountManager::getInstance().checkAndSignalForAccessToken();        
    }
}
