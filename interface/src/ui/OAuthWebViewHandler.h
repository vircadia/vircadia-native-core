//
//  OAuthWebviewHandler.h
//  interface/src/ui
//
//  Created by Stephen Birarda on 2014-05-01.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OAuthWebviewHandler_h
#define hifi_OAuthWebviewHandler_h

#include <QtCore/QUrl>

class QWebView;

class OAuthWebViewHandler : public QObject {
    Q_OBJECT
public:
    OAuthWebViewHandler();
    static OAuthWebViewHandler& getInstance();
    static void addHighFidelityRootCAToSSLConfig();
    
    void clearLastAuthorizationURL() { _lastAuthorizationURL = QUrl(); }
    
public slots:
    void displayWebviewForAuthorizationURL(const QUrl& authorizationURL);
    
private slots:
    void handleSSLErrors(QNetworkReply* networkReply, const QList<QSslError>& errorList);
    void handleLoadFinished(bool success);
    void handleReplyFinished(QNetworkReply* reply);
    void handleWebViewDestroyed(QObject* destroyedObject);
    void handleURLChanged(const QUrl& url);
private:
    QPointer<QWebView> _activeWebView;
    QElapsedTimer _webViewRedisplayTimer;
    QUrl _lastAuthorizationURL;
};

#endif // hifi_OAuthWebviewHandler_h