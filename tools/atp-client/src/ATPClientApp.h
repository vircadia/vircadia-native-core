//
//  ATPClientApp.h
//  tools/atp-client/src
//
//  Created by Seth Alves on 2017-3-15
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_ATPClientApp_h
#define hifi_ATPClientApp_h

#include <QCoreApplication>
#include <udt/Constants.h>
#include <udt/Socket.h>
#include <ReceivedMessage.h>
#include <NetworkPeer.h>
#include <NodeList.h>
#include <AssetRequest.h>
#include <MappingRequest.h>


class ATPClientApp : public QCoreApplication {
    Q_OBJECT
public:
    ATPClientApp(int argc, char* argv[]);
    ~ATPClientApp();

private slots:
    void domainConnectionRefused(const QString& reasonMessage, int reasonCodeInt, const QString& extraInfo);
    void domainChanged(const QString& domainHostname);
    void nodeAdded(SharedNodePointer node);
    void nodeActivated(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
    void notifyPacketVersionMismatch();

private:
    void go();
    NodeList* _nodeList;
    void timedOut();
    void uploadAsset();
    void setMapping(QString hash);
    void lookupAsset();
    void listAssets();
    void download(AssetUtils::AssetHash hash);
    void finish(int exitCode);
    bool _verbose;

    QUrl _url;
    QString _localOutputFile;
    QString _localUploadFile;

    int _listenPort { INVALID_PORT };

    QString _domainServerAddress;

    QString _username;
    QString _password;

    bool _waitingForLogin { false };
    bool _waitingForNode { true };

    QTimer* _domainCheckInTimer { nullptr };
    QTimer* _timeoutTimer { nullptr };
};

#endif // hifi_ATPClientApp_h
