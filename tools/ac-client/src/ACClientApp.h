//
//  ACClientApp.h
//  tools/ac-client/src
//
//  Created by Seth Alves on 2016-10-5
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_ACClientApp_h
#define hifi_ACClientApp_h

#include <QApplication>
#include <udt/Constants.h>
#include <udt/Socket.h>
#include <ReceivedMessage.h>
#include <NetworkPeer.h>
#include <NodeList.h>


class ACClientApp : public QCoreApplication {
    Q_OBJECT
public:
    ACClientApp(int argc, char* argv[]);
    ~ACClientApp();

    const int stunFailureExitStatus { 1 };
    const int iceFailureExitStatus { 2 };
    const int domainPingExitStatus { 3 };

private slots:
    void domainConnectionRefused(const QString& reasonMessage, int reasonCodeInt, const QString& extraInfo);
    void domainChanged(const QString& domainHostname);
    void nodeAdded(SharedNodePointer node);
    void nodeActivated(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);

private:
    NodeList* _nodeList;
    void timedOut();
    void finish(int exitCode);
    bool _verbose;
    QTimer* _pingDomainTimer { nullptr };

    bool _sawEntityServer { false };
    bool _sawAudioMixer { false };
    bool _sawAvatarMixer { false };
    bool _sawAssetServer { false };
    bool _sawMessagesMixer { false };
};

#endif //hifi_ACClientApp_h
