//
//  ATPGetApp.h
//  tools/atp-get/src
//
//  Created by Seth Alves on 2017-3-15
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_ATPGetApp_h
#define hifi_ATPGetApp_h

#include <QApplication>
#include <udt/Constants.h>
#include <udt/Socket.h>
#include <ReceivedMessage.h>
#include <NetworkPeer.h>
#include <NodeList.h>
#include <AssetRequest.h>
#include <MappingRequest.h>


class ATPGetApp : public QCoreApplication {
    Q_OBJECT
public:
    ATPGetApp(int argc, char* argv[]);
    ~ATPGetApp();

private slots:
    void domainConnectionRefused(const QString& reasonMessage, int reasonCodeInt, const QString& extraInfo);
    void domainChanged(const QString& domainHostname);
    void nodeAdded(SharedNodePointer node);
    void nodeActivated(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
    void notifyPacketVersionMismatch();

private:
    NodeList* _nodeList;
    void timedOut();
    void lookup();
    void download(AssetHash hash);
    void finish(int exitCode);
    bool _verbose;

    QUrl _url;
    QString _localOutputFile;
};

#endif // hifi_ATPGetApp_h
