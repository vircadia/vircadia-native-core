//
//  AudioMixerDatagramProcessor.h
//  assignment-client/src
//
//  Created by Stephen Birarda on 2014-08-14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioMixerDatagramProcessor_h
#define hifi_AudioMixerDatagramProcessor_h

#include <qobject.h>
#include <qudpsocket.h>

class AudioMixerDatagramProcessor : public QObject {
    Q_OBJECT
public:
    AudioMixerDatagramProcessor(QUdpSocket& nodeSocket, QThread* previousNodeSocketThread);
    ~AudioMixerDatagramProcessor();
public slots:
    void readPendingDatagrams();
signals:
    void packetRequiresProcessing(const QByteArray& receivedPacket, const HifiSockAddr& senderSockAddr);
private:
    QUdpSocket& _nodeSocket;
    QThread* _previousNodeSocketThread;
};

#endif // hifi_AudioMixerDatagramProcessor_h