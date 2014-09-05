//
//  XMLHttpRequestClass.h
//  libraries/script-engine/src/
//
//  Created by Ryan Huffman on 5/2/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_XMLHttpRequestClass_h
#define hifi_XMLHttpRequestClass_h

#include <QBuffer>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QScriptContext>
#include <QScriptEngine>
#include <QScriptValue>
#include <QTimer>

class XMLHttpRequestClass : public QObject {
    Q_OBJECT
    Q_PROPERTY(QScriptValue response READ getResponse)
    Q_PROPERTY(QScriptValue responseText READ getResponseText)
    Q_PROPERTY(QString responseType READ getResponseType WRITE setResponseType)
    Q_PROPERTY(QScriptValue status READ getStatus)
    Q_PROPERTY(QString statusText READ getStatusText)
    Q_PROPERTY(QScriptValue readyState READ getReadyState)
    Q_PROPERTY(QScriptValue errorCode READ getError)
    Q_PROPERTY(int timeout READ getTimeout WRITE setTimeout)

    Q_PROPERTY(int UNSENT READ getUnsent)
    Q_PROPERTY(int OPENED READ getOpened)
    Q_PROPERTY(int HEADERS_RECEIVED READ getHeadersReceived)
    Q_PROPERTY(int LOADING READ getLoading)
    Q_PROPERTY(int DONE READ getDone)

    // Callbacks
    Q_PROPERTY(QScriptValue ontimeout READ getOnTimeout WRITE setOnTimeout)
    Q_PROPERTY(QScriptValue onreadystatechange READ getOnReadyStateChange WRITE setOnReadyStateChange)
public:
    XMLHttpRequestClass(QScriptEngine* engine);
    ~XMLHttpRequestClass();

    static const int MAXIMUM_REDIRECTS = 5;
    enum ReadyState {
        UNSENT = 0,
        OPENED,
        HEADERS_RECEIVED,
        LOADING,
        DONE
    };

    int getUnsent() const { return UNSENT; };
    int getOpened() const { return OPENED; };
    int getHeadersReceived() const { return HEADERS_RECEIVED; };
    int getLoading() const { return LOADING; };
    int getDone() const { return DONE; };

    static QScriptValue constructor(QScriptContext* context, QScriptEngine* engine);

    int getTimeout() const { return _timeout; }
    void setTimeout(int timeout) { _timeout = timeout; }
    QScriptValue getResponse() const { return _responseData; }
    QScriptValue getResponseText() const { return QScriptValue(QString(_rawResponseData.data())); }
    QString getResponseType() const { return _responseType; }
    void setResponseType(const QString& responseType) { _responseType = responseType; }
    QScriptValue getReadyState() const { return QScriptValue(_readyState); }
    QScriptValue getError() const { return QScriptValue(_errorCode); }
    QScriptValue getStatus() const;
    QString getStatusText() const;

    QScriptValue getOnTimeout() const { return _onTimeout; }
    void setOnTimeout(QScriptValue function) { _onTimeout = function; }
    QScriptValue getOnReadyStateChange() const { return _onReadyStateChange; }
    void setOnReadyStateChange(QScriptValue function) { _onReadyStateChange = function; }

public slots:
    void abort();
    void setRequestHeader(const QString& name, const QString& value);
    void open(const QString& method, const QString& url, bool async = true, const QString& username = "",
              const QString& password = "");
    void send();
    void send(const QScriptValue& data);
    QScriptValue getAllResponseHeaders() const;
    QScriptValue getResponseHeader(const QString& name) const;

signals:
    void requestComplete();

private:
    void setReadyState(ReadyState readyState);
    void doSend();
    void connectToReply(QNetworkReply* reply);
    void disconnectFromReply(QNetworkReply* reply);
    void abortRequest();
    void notImplemented();

    QScriptEngine* _engine;
    bool _async;
    QUrl _url;
    QString _method;
    QString _responseType;
    QNetworkRequest _request;
    QNetworkReply* _reply;
    QBuffer* _sendData;
    QByteArray _rawResponseData;
    QScriptValue _responseData;
    QScriptValue _onTimeout;
    QScriptValue _onReadyStateChange;
    ReadyState _readyState;
    QNetworkReply::NetworkError _errorCode;
    QFile* _file;
    int _timeout;
    QTimer _timer;
    int _numRedirects;

private slots:
    void requestFinished();
    void requestError(QNetworkReply::NetworkError code);
    void requestMetaDataChanged();
    void requestDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void requestTimeout();
};

#endif // hifi_XMLHttpRequestClass_h
