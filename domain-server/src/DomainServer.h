//
//  DomainServer.h
//  domain-server/src
//
//  Created by Stephen Birarda on 9/26/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainServer_h
#define hifi_DomainServer_h

#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QJsonObject>
#include <QtCore/QQueue>
#include <QtCore/QSharedPointer>
#include <QtCore/QStringList>
#include <QtCore/QUrl>
#include <QAbstractNativeEventFilter>

#include <Assignment.h>
#include <HTTPSConnection.h>
#include <LimitedNodeList.h>

#include "DomainGatekeeper.h"
#include "DomainServerSettingsManager.h"
#include "DomainServerWebSessionData.h"
#include "WalletTransaction.h"

#include "PendingAssignedNodeData.h"

typedef QSharedPointer<Assignment> SharedAssignmentPointer;
typedef QMultiHash<QUuid, WalletTransaction*> TransactionHash;

class DomainServer : public QCoreApplication, public HTTPSRequestHandler {
    Q_OBJECT
public:
    DomainServer(int argc, char* argv[]);
    ~DomainServer();
    
    static int const EXIT_CODE_REBOOT;

    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false);
    bool handleHTTPSRequest(HTTPSConnection* connection, const QUrl& url, bool skipSubHandler = false);

public slots:
    /// Called by NodeList to inform us a node has been added
    void nodeAdded(SharedNodePointer node);
    /// Called by NodeList to inform us a node has been killed
    void nodeKilled(SharedNodePointer node);

    void transactionJSONCallback(const QJsonObject& data);

    void restart();

    void processRequestAssignmentPacket(QSharedPointer<NLPacket> packet);
    void processListRequestPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode);
    void processNodeJSONStatsPacket(QSharedPointer<NLPacket> packet, SharedNodePointer sendingNode);
    void processPathQueryPacket(QSharedPointer<NLPacket> packet);

private slots:
    void aboutToQuit();

    void loginFailed();
    void setupPendingAssignmentCredits();
    void sendPendingTransactionsToServer();

    void performIPAddressUpdate(const HifiSockAddr& newPublicSockAddr);
    void sendHeartbeatToDataServer() { sendHeartbeatToDataServer(QString()); }
    void sendHeartbeatToIceServer();
    
    void handleConnectedNode(SharedNodePointer newNode);
    
private:
    void setupNodeListAndAssignments(const QUuid& sessionUUID = QUuid::createUuid());
    bool optionallySetupOAuth();
    bool optionallyReadX509KeyAndCertificate();
    bool optionallySetupAssignmentPayment();

    bool didSetupAccountManagerWithAccessToken();
    bool resetAccountManagerAccessToken();

    void setupAutomaticNetworking();
    void sendHeartbeatToDataServer(const QString& networkAddress);

    unsigned int countConnectedUsers();

    void sendDomainListToNode(const SharedNodePointer& node, const HifiSockAddr& senderSockAddr);

    QUuid connectionSecretForNodes(const SharedNodePointer& nodeA, const SharedNodePointer& nodeB);
    void broadcastNewNode(const SharedNodePointer& node);

    void parseAssignmentConfigs(QSet<Assignment::Type>& excludedTypes);
    void addStaticAssignmentToAssignmentHash(Assignment* newAssignment);
    void createStaticAssignmentsForType(Assignment::Type type, const QVariantList& configList);
    void populateDefaultStaticAssignmentsExcludingTypes(const QSet<Assignment::Type>& excludedTypes);
    void populateStaticScriptedAssignmentsFromSettings();

    SharedAssignmentPointer dequeueMatchingAssignment(const QUuid& checkInUUID, NodeType_t nodeType);
    SharedAssignmentPointer deployableAssignmentForRequest(const Assignment& requestAssignment);
    void refreshStaticAssignmentAndAddToQueue(SharedAssignmentPointer& assignment);
    void addStaticAssignmentsToQueue();
    
    QUrl oauthRedirectURL();
    QUrl oauthAuthorizationURL(const QUuid& stateUUID = QUuid::createUuid());

    bool isAuthenticatedRequest(HTTPConnection* connection, const QUrl& url);

    void handleTokenRequestFinished();
    QNetworkReply* profileRequestGivenTokenReply(QNetworkReply* tokenReply);
    void handleProfileRequestFinished();
    Headers setupCookieHeadersFromProfileReply(QNetworkReply* profileReply);

    void loadExistingSessionsFromSettings();

    QJsonObject jsonForSocket(const HifiSockAddr& socket);
    QJsonObject jsonObjectForNode(const SharedNodePointer& node);
    
    DomainGatekeeper _gatekeeper;

    HTTPManager _httpManager;
    HTTPSManager* _httpsManager;

    QHash<QUuid, SharedAssignmentPointer> _allAssignments;
    QQueue<SharedAssignmentPointer> _unfulfilledAssignments;
    TransactionHash _pendingAssignmentCredits;

    bool _isUsingDTLS;

    QUrl _oauthProviderURL;
    QString _oauthClientID;
    QString _oauthClientSecret;
    QString _hostname;

    QSet<QUuid> _webAuthenticationStateSet;
    QHash<QUuid, DomainServerWebSessionData> _cookieSessionHash;

    QString _automaticNetworkingSetting;

    DomainServerSettingsManager _settingsManager;

    HifiSockAddr _iceServerSocket;
    
    friend class DomainGatekeeper;
};


#endif // hifi_DomainServer_h
