//
//  OctreeServer.h
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeServer_h
#define hifi_OctreeServer_h

#include <memory>

#include <QStringList>
#include <QDateTime>
#include <QtCore/QCoreApplication>

#include <HTTPManager.h>

#include <ThreadedAssignment.h>

#include "OctreePersistThread.h"
#include "OctreeSendThread.h"
#include "OctreeServerConsts.h"
#include "OctreeInboundPacketProcessor.h"

const int DEFAULT_PACKETS_PER_INTERVAL = 2000; // some 120,000 packets per second total

/// Handles assignments of type OctreeServer - sending octrees to various clients.
class OctreeServer : public ThreadedAssignment, public HTTPRequestHandler {
    Q_OBJECT
public:
    OctreeServer(ReceivedMessage& message);
    ~OctreeServer();

    /// allows setting of run arguments
    void setArguments(int argc, char** argv);

    bool wantsDebugSending() const { return _debugSending; }
    bool wantsDebugReceiving() const { return _debugReceiving; }
    bool wantsVerboseDebug() const { return _verboseDebug; }

    OctreePointer getOctree() { return _tree; }
    JurisdictionMap* getJurisdiction() { return _jurisdiction; }

    int getPacketsPerClientPerInterval() const { return std::min(_packetsPerClientPerInterval,
                                std::max(1, getPacketsTotalPerInterval() / std::max(1, getCurrentClientCount()))); }

    int getPacketsPerClientPerSecond() const { return getPacketsPerClientPerInterval() * INTERVALS_PER_SECOND; }
    int getPacketsTotalPerInterval() const { return _packetsTotalPerInterval; }
    int getPacketsTotalPerSecond() const { return getPacketsTotalPerInterval() * INTERVALS_PER_SECOND; }

    static int getCurrentClientCount() { return _clientCount; }
    static void clientConnected() { _clientCount++; }
    static void clientDisconnected() { _clientCount--; }

    bool isInitialLoadComplete() const { return (_persistThread) ? _persistThread->isInitialLoadComplete() : true; }
    bool isPersistEnabled() const { return (_persistThread) ? true : false; }
    quint64 getLoadElapsedTime() const { return (_persistThread) ? _persistThread->getLoadElapsedTime() : 0; }
    QString getPersistFilename() const { return (_persistThread) ? _persistThread->getPersistFilename() : ""; }
    QString getPersistFileMimeType() const { return (_persistThread) ? _persistThread->getPersistFileMimeType() : "text/plain"; }
    QByteArray getPersistFileContents() const { return (_persistThread) ? _persistThread->getPersistFileContents() : QByteArray(); }

    // Subclasses must implement these methods
    virtual std::unique_ptr<OctreeQueryNode> createOctreeQueryNode() = 0;
    virtual char getMyNodeType() const = 0;
    virtual PacketType getMyQueryMessageType() const = 0;
    virtual const char* getMyServerName() const = 0;
    virtual const char* getMyLoggingServerTargetName() const = 0;
    virtual const char* getMyDefaultPersistFilename() const = 0;
    virtual PacketType getMyEditNackType() const = 0;
    virtual QString getMyDomainSettingsKey() const { return QString("octree_server_settings"); }

    // subclass may implement these method
    virtual void beforeRun() { }
    virtual bool hasSpecialPacketsToSend(const SharedNodePointer& node) { return false; }
    virtual int sendSpecialPackets(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent) { return 0; }
    virtual QString serverSubclassStats() { return QString(); }
    virtual void trackSend(const QUuid& dataID, quint64 dataLastEdited, const QUuid& viewerNode) { }
    virtual void trackViewerGone(const QUuid& viewerNode) { }

    static float SKIP_TIME; // use this for trackXXXTime() calls for non-times

    static void trackLoopTime(float time) { _averageLoopTime.updateAverage(time); }
    static float getAverageLoopTime() { return _averageLoopTime.getAverage(); }

    static void trackEncodeTime(float time);
    static float getAverageEncodeTime() { return _averageEncodeTime.getAverage(); }

    static void trackInsideTime(float time) { _averageInsideTime.updateAverage(time); }
    static float getAverageInsideTime() { return _averageInsideTime.getAverage(); }

    static void trackTreeWaitTime(float time);
    static float getAverageTreeWaitTime() { return _averageTreeWaitTime.getAverage(); }

    static void trackNodeWaitTime(float time) { _averageNodeWaitTime.updateAverage(time); }
    static float getAverageNodeWaitTime() { return _averageNodeWaitTime.getAverage(); }

    static void trackCompressAndWriteTime(float time);
    static float getAverageCompressAndWriteTime() { return _averageCompressAndWriteTime.getAverage(); }

    static void trackPacketSendingTime(float time);
    static float getAveragePacketSendingTime() { return _averagePacketSendingTime.getAverage(); }

    static void trackProcessWaitTime(float time);
    static float getAverageProcessWaitTime() { return _averageProcessWaitTime.getAverage(); }

    // these methods allow us to track which threads got to various states
    static void didProcess(OctreeSendThread* thread);
    static void didPacketDistributor(OctreeSendThread* thread);
    static void didHandlePacketSend(OctreeSendThread* thread);
    static void didCallWriteDatagram(OctreeSendThread* thread);
    static void stopTrackingThread(OctreeSendThread* thread);

    static int howManyThreadsDidProcess(quint64 since = 0);
    static int howManyThreadsDidPacketDistributor(quint64 since = 0);
    static int howManyThreadsDidHandlePacketSend(quint64 since = 0);
    static int howManyThreadsDidCallWriteDatagram(quint64 since = 0);

    bool handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) override;

    virtual void aboutToFinish() override;

public slots:
    /// runs the octree server assignment
    void run() override;
    virtual void nodeAdded(SharedNodePointer node);
    virtual void nodeKilled(SharedNodePointer node);
    void sendStatsPacket() override;

private slots:
    void domainSettingsRequestComplete();
    void handleOctreeQueryPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleOctreeDataNackPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void handleJurisdictionRequestPacket(QSharedPointer<ReceivedMessage> message, SharedNodePointer senderNode);
    void removeSendThread();

protected:
    using UniqueSendThread = std::unique_ptr<OctreeSendThread>;
    using SendThreads = std::unordered_map<QUuid, UniqueSendThread>;
    
    virtual OctreePointer createTree() = 0;
    bool readOptionBool(const QString& optionName, const QJsonObject& settingsSectionObject, bool& result);
    bool readOptionInt(const QString& optionName, const QJsonObject& settingsSectionObject, int& result);
    bool readOptionInt64(const QString& optionName, const QJsonObject& settingsSectionObject, qint64& result);
    bool readOptionString(const QString& optionName, const QJsonObject& settingsSectionObject, QString& result);
    void readConfiguration();
    virtual void readAdditionalConfiguration(const QJsonObject& settingsSectionObject) { };
    void parsePayload();
    void initHTTPManager(int port);
    void resetSendingStats();
    QString getUptime();
    QString getFileLoadTime();
    QString getConfiguration();
    QString getStatusLink();
    
    UniqueSendThread createSendThread(const SharedNodePointer& node);

    int _argc;
    const char** _argv;
    char** _parsedArgV;
    QJsonObject _settings;

    bool _isShuttingDown = false;

    HTTPManager* _httpManager;
    int _statusPort;
    QString _statusHost;

    QString _persistFilePath;
    QString _persistAsFileType;
    QString _backupDirectoryPath;
    int _packetsPerClientPerInterval;
    int _packetsTotalPerInterval;
    OctreePointer _tree; // this IS a reaveraging tree
    bool _wantPersist;
    bool _debugSending;
    bool _debugReceiving;
    bool _debugTimestampNow;
    bool _verboseDebug;
    JurisdictionMap* _jurisdiction;
    JurisdictionSender* _jurisdictionSender;
    OctreeInboundPacketProcessor* _octreeInboundPacketProcessor;
    OctreePersistThread* _persistThread;

    int _persistInterval;
    bool _wantBackup;
    bool _persistFileDownload;
    QString _backupExtensionFormat;
    int _backupInterval;
    int _maxBackupVersions;

    time_t _started;
    quint64 _startedUSecs;
    QString _safeServerName;
    
    SendThreads _sendThreads;

    static int _clientCount;
    static SimpleMovingAverage _averageLoopTime;

    static SimpleMovingAverage _averageEncodeTime;
    static SimpleMovingAverage _averageShortEncodeTime;
    static SimpleMovingAverage _averageLongEncodeTime;
    static SimpleMovingAverage _averageExtraLongEncodeTime;
    static int _extraLongEncode;
    static int _longEncode;
    static int _shortEncode;
    static int _noEncode;

    static SimpleMovingAverage _averageInsideTime;

    static SimpleMovingAverage _averageTreeWaitTime;
    static SimpleMovingAverage _averageTreeShortWaitTime;
    static SimpleMovingAverage _averageTreeLongWaitTime;
    static SimpleMovingAverage _averageTreeExtraLongWaitTime;
    static int _extraLongTreeWait;
    static int _longTreeWait;
    static int _shortTreeWait;
    static int _noTreeWait;

    static SimpleMovingAverage _averageNodeWaitTime;

    static SimpleMovingAverage _averageCompressAndWriteTime;
    static SimpleMovingAverage _averageShortCompressTime;
    static SimpleMovingAverage _averageLongCompressTime;
    static SimpleMovingAverage _averageExtraLongCompressTime;
    static int _extraLongCompress;
    static int _longCompress;
    static int _shortCompress;
    static int _noCompress;

    static SimpleMovingAverage _averagePacketSendingTime;
    static int _noSend;

    static SimpleMovingAverage _averageProcessWaitTime;
    static SimpleMovingAverage _averageProcessShortWaitTime;
    static SimpleMovingAverage _averageProcessLongWaitTime;
    static SimpleMovingAverage _averageProcessExtraLongWaitTime;
    static int _extraLongProcessWait;
    static int _longProcessWait;
    static int _shortProcessWait;
    static int _noProcessWait;

    static QMap<OctreeSendThread*, quint64> _threadsDidProcess;
    static QMap<OctreeSendThread*, quint64> _threadsDidPacketDistributor;
    static QMap<OctreeSendThread*, quint64> _threadsDidHandlePacketSend;
    static QMap<OctreeSendThread*, quint64> _threadsDidCallWriteDatagram;

    static QMutex _threadsDidProcessMutex;
    static QMutex _threadsDidPacketDistributorMutex;
    static QMutex _threadsDidHandlePacketSendMutex;
    static QMutex _threadsDidCallWriteDatagramMutex;
};

#endif // hifi_OctreeServer_h
