//
//  AudioInjector.h
//  hifi
//
//  Created by Stephen Birarda on 12/19/2013.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__AudioInjector__
#define __hifi__AudioInjector__

#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QUrl>

class AbstractAudioInterface;
class QNetworkReply;

class AudioInjector : public QObject {
    Q_OBJECT
public:
    AudioInjector(const QUrl& sampleURL);
    
    int size() const { return _sampleByteArray.size(); }
public slots:
    void injectViaThread(AbstractAudioInterface* localAudioInterface = NULL);
    
private:
    QByteArray _sampleByteArray;
    QThread _thread;
    QUrl _sourceURL;
    
private slots:
    void startDownload();
    void replyFinished(QNetworkReply* reply);
    void injectAudio(AbstractAudioInterface* localAudioInterface);
};

#endif /* defined(__hifi__AudioInjector__) */
