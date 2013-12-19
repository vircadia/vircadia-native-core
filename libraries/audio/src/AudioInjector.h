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

class AbstractAudioInterface;
class QNetworkReply;

class AudioInjector : public QObject {
    Q_OBJECT
public:
    AudioInjector(const QUrl& sampleURL, QObject* parent = 0);
    
    void injectViaThread(AbstractAudioInterface* localAudioInterface = NULL);
    
private:
    QByteArray _sampleByteArray;
private slots:
    void replyFinished(QNetworkReply* reply);
};

#endif /* defined(__hifi__AudioInjector__) */
