//
//  ModelUploader.h
//  hifi
//
//  Created by Clément Brisset on 3/4/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ModelUploader__
#define __hifi__ModelUploader__

#include <QTimer>

class QDialog;
class QFileInfo;
class QHttpMultiPart;
class QProgressBar;

class TemporaryDir;

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
    bool addTextures(const QFileInfo& texdir);
    bool addPart(const QString& path, const QString& name);
};

#endif /* defined(__hifi__ModelUploader__) */
