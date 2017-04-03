//
//  SpeechRecognitionScriptingInterface.h
//  interface/src/scripting
//
//  Created by Trevor Berninger on 3/24/17.
//  Copyright 2017 Limitless ltd.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_LimitlessConnection_h
#define hifi_LimitlessConnection_h

#include <AudioClient.h>
#include <QObject>
#include <QFuture>

class LimitlessConnection : public QObject {
    Q_OBJECT
public:
    LimitlessConnection();

    Q_INVOKABLE void startListening(QString authCode);
    Q_INVOKABLE void stopListening();

    std::atomic<bool> _streamingAudioForTranscription;

signals:
    void onReceivedTranscription(QString speech);
    void onFinishedSpeaking(QString speech);

private:
    void transcriptionReceived();
    void audioInputReceived(const QByteArray& inputSamples);

    bool isConnected() const;

    std::unique_ptr<QTcpSocket> _transcribeServerSocket;
    QByteArray _serverDataBuffer;
    QString _currentTranscription;
};

#endif //hifi_LimitlessConnection_h
