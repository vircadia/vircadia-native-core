//
//  ModelUploader.h
//  interface/src
//
//  Created by Clément Brisset on 3/4/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ModelUploader_h
#define hifi_ModelUploader_h

#include <QTimer>

class QDialog;
class QFileInfo;
class QHttpMultiPart;
class QProgressBar;

class ModelUploader : public QObject {
    Q_OBJECT
    
public:
    ModelUploader(bool isHead);
    ~ModelUploader();
    
public slots:
    void send();
    
private slots:
    void uploadUpdate(qint64 bytesSent, qint64 bytesTotal);
    void uploadSuccess(const QJsonObject& jsonResponse);
    void uploadFailed(QNetworkReply::NetworkError errorCode, const QString& errorString);
    void checkS3();
    void processCheck();
    
private:
    QString _url;
    int _lodCount;
    int _texturesCount;
    int _totalSize;
    bool _isHead;
    bool _readyToSend;
    
    QHttpMultiPart* _dataMultiPart;
    QNetworkAccessManager _networkAccessManager;
    
    int _numberOfChecks;
    QTimer _timer;
    
    QDialog* _progressDialog;
    QProgressBar* _progressBar;
    
    
    bool zip();
    bool addTextures(const QString& texdir, const QString fbxFile);
    bool addPart(const QString& path, const QString& name);
};

#endif // hifi_ModelUploader_h
