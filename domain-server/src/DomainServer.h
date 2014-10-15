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

#include "DomainServerSettingsManager.h"
#include "DomainServerWebSessionData.h"
#include "ShutdownEventListener.h"
#include "WalletTransaction.h"

#include "PendingAssignedNodeData.h"

typedef QSharedPointer<Assignment> SharedAssignmentPointer;
typedef QMultiHash<QUuid, WalletTransaction*> TransactionHash;


class DomainServer : public QCoreApplication, public HTTPSRequestHandler {
    Q_OBJECT
public:
    DomainServer(int argc, char* argv[]);
    
    static int const EXIT_CODE_REBOOT;
    
    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler = false);
    bool handleHTTPSRequest(HTTPSConnection* connection, const QUrl& url, bool skipSubHandler = false);
    
public slots:
    /// Called by NodeList to inform us a node has been added
    void nodeAdded(SharedNodePointer node);
    /// Called by NodeList to inform us a node has been killed
    void nodeKilled(SharedNodePointer node);
    
    void publicKeyJSONCallback(QNetworkReply& requestReply);
    void transactionJSONCallback(const QJsonObject& data);
    
    void restart();
    
private slots:
    void loginFailed();
    void readAvailableDatagrams();
    void setupPendingAssignmentCredits();
    void sendPendingTransactionsToServer();
    
    void requestCurrentPublicSocketViaSTUN();
    void performIPAddressUpdate(const HifiSockAddr& newPublicSockAddr);
    void performICEUpdates();
    void sendHearbeatToIceServer();
    void sendICEPingPackets();
private:
    void setupNodeListAndAssignments(const QUuid& sessionUUID = QUuid::createUuid());
    bool optionallySetupOAuth();
    bool optionallyReadX509KeyAndCertificate();
    bool didSetupAccountManagerWithAccessToken();
    bool optionallySetupAssignmentPayment();
    
    void setupAutomaticNetworking();
    void updateNetworkingInfoWithDataServer(const QString& newSetting, const QString& networkAddress = QString());
    void processICEPingReply(const QByteArray& packet, const HifiSockAddr& senderSockAddr);
    void processICEHeartbeatResponse(const QByteArray& packet);
    
    void processDatagram(const QByteArray& receivedPacket, const HifiSockAddr& senderSockAddr);
    
    void handleConnectRequest(const QByteArray& packet, const HifiSockAddr& senderSockAddr);
    bool shouldAllowConnectionFromNode(const QString& username, const QByteArray& usernameSignature,
                                       const HifiSockAddr& senderSockAddr);
    int parseNodeDataFromByteArray(QDataStream& packetStream,
                                   NodeType_t& nodeType,
                                   HifiSockAddr& publicSockAddr,
                                   HifiSockAddr& localSockAddr,
                                   const HifiSockAddr& senderSockAddr);
    NodeSet nodeInterestListFromPacket(const QByteArray& packet, int numPreceedingBytes);
    void sendDomainListToNode(const SharedNodePointer& node, const HifiSockAddr& senderSockAddr,
                              const NodeSet& nodeInterestList);
    
    void parseAssignmentConfigs(QSet<Assignment::Type>& excludedTypes);
    void addStaticAssignmentToAssignmentHash(Assignment* newAssignment);
    void createScriptedAssignmentsFromList(const QVariantList& configList);
    void createStaticAssignmentsForType(Assignment::Type type, const QVariantList& configList);
    void populateDefaultStaticAssignmentsExcludingTypes(const QSet<Assignment::Type>& excludedTypes);
    
    SharedAssignmentPointer matchingQueuedAssignmentForCheckIn(const QUuid& checkInUUID, NodeType_t nodeType);
    SharedAssignmentPointer deployableAssignmentForRequest(const Assignment& requestAssignment);
    void removeMatchingAssignmentFromQueue(const SharedAssignmentPointer& removableAssignment);
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

    ShutdownEventListener _shutdownEventListener;
    
    HTTPManager _httpManager;
    HTTPSManager* _httpsManager;
    
    QHash<QUuid, SharedAssignmentPointer> _allAssignments;
    QQueue<SharedAssignmentPointer> _unfulfilledAssignments;
    QHash<QUuid, PendingAssignedNodeData*> _pendingAssignedNodes;
    TransactionHash _pendingAssignmentCredits;
    
    bool _isUsingDTLS;
    
    QUrl _oauthProviderURL;
    QString _oauthClientID;
    QString _oauthClientSecret;
    QString _hostname;
    
    QSet<QUuid> _webAuthenticationStateSet;
    QHash<QUuid, DomainServerWebSessionData> _cookieSessionHash;
    
    QHash<QString, QByteArray> _userPublicKeys;
    
    QHash<QUuid, NetworkPeer> _connectingICEPeers;
    QHash<QUuid, HifiSockAddr> _connectedICEPeers;
    
    DomainServerSettingsManager _settingsManager;
};


#endif // hifi_DomainServer_h
