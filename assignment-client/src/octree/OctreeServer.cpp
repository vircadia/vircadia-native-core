//
//  OctreeServer.cpp
//  assignment-client/src/octree
//
//  Created by Brad Hefta-Gaub on 9/16/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QUuid>

#include <time.h>

#include <AccountManager.h>
#include <HTTPConnection.h>
#include <LogHandler.h>
#include <NetworkingConstants.h>
#include <NumericalConstants.h>
#include <UUID.h>

#include "../AssignmentClient.h"

#include "OctreeServer.h"
#include "OctreeServerConsts.h"

OctreeServer* OctreeServer::_instance = NULL;
int OctreeServer::_clientCount = 0;
const int MOVING_AVERAGE_SAMPLE_COUNTS = 1000000;

float OctreeServer::SKIP_TIME = -1.0f; // use this for trackXXXTime() calls for non-times

SimpleMovingAverage OctreeServer::_averageLoopTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageInsideTime(MOVING_AVERAGE_SAMPLE_COUNTS);

SimpleMovingAverage OctreeServer::_averageEncodeTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageShortEncodeTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageLongEncodeTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageExtraLongEncodeTime(MOVING_AVERAGE_SAMPLE_COUNTS);
int OctreeServer::_extraLongEncode = 0;
int OctreeServer::_longEncode = 0;
int OctreeServer::_shortEncode = 0;
int OctreeServer::_noEncode = 0;

SimpleMovingAverage OctreeServer::_averageTreeWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageTreeShortWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageTreeLongWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageTreeExtraLongWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);
int OctreeServer::_extraLongTreeWait = 0;
int OctreeServer::_longTreeWait = 0;
int OctreeServer::_shortTreeWait = 0;
int OctreeServer::_noTreeWait = 0;

SimpleMovingAverage OctreeServer::_averageNodeWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);

SimpleMovingAverage OctreeServer::_averageCompressAndWriteTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageShortCompressTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageLongCompressTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageExtraLongCompressTime(MOVING_AVERAGE_SAMPLE_COUNTS);
int OctreeServer::_extraLongCompress = 0;
int OctreeServer::_longCompress = 0;
int OctreeServer::_shortCompress = 0;
int OctreeServer::_noCompress = 0;

SimpleMovingAverage OctreeServer::_averagePacketSendingTime(MOVING_AVERAGE_SAMPLE_COUNTS);
int OctreeServer::_noSend = 0;

SimpleMovingAverage OctreeServer::_averageProcessWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageProcessShortWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageProcessLongWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);
SimpleMovingAverage OctreeServer::_averageProcessExtraLongWaitTime(MOVING_AVERAGE_SAMPLE_COUNTS);
int OctreeServer::_extraLongProcessWait = 0;
int OctreeServer::_longProcessWait = 0;
int OctreeServer::_shortProcessWait = 0;
int OctreeServer::_noProcessWait = 0;


void OctreeServer::resetSendingStats() {
    _averageLoopTime.reset();

    _averageEncodeTime.reset();
    _averageShortEncodeTime.reset();
    _averageLongEncodeTime.reset();
    _averageExtraLongEncodeTime.reset();
    _extraLongEncode = 0;
    _longEncode = 0;
    _shortEncode = 0;
    _noEncode = 0;

    _averageInsideTime.reset();
    _averageTreeWaitTime.reset();
    _averageTreeShortWaitTime.reset();
    _averageTreeLongWaitTime.reset();
    _averageTreeExtraLongWaitTime.reset();
    _extraLongTreeWait = 0;
    _longTreeWait = 0;
    _shortTreeWait = 0;
    _noTreeWait = 0;

    _averageNodeWaitTime.reset();

    _averageCompressAndWriteTime.reset();
    _averageShortCompressTime.reset();
    _averageLongCompressTime.reset();
    _averageExtraLongCompressTime.reset();
    _extraLongCompress = 0;
    _longCompress = 0;
    _shortCompress = 0;
    _noCompress = 0;

    _averagePacketSendingTime.reset();
    _noSend = 0;

    _averageProcessWaitTime.reset();
    _averageProcessShortWaitTime.reset();
    _averageProcessLongWaitTime.reset();
    _averageProcessExtraLongWaitTime.reset();
    _extraLongProcessWait = 0;
    _longProcessWait = 0;
    _shortProcessWait = 0;
    _noProcessWait = 0;
}

void OctreeServer::trackEncodeTime(float time) {
    const float MAX_SHORT_TIME = 10.0f;
    const float MAX_LONG_TIME = 100.0f;

    if (time == SKIP_TIME) {
        _noEncode++;
        time = 0.0f;
    } else if (time <= MAX_SHORT_TIME) {
        _shortEncode++;
        _averageShortEncodeTime.updateAverage(time);
    } else if (time <= MAX_LONG_TIME) {
        _longEncode++;
        _averageLongEncodeTime.updateAverage(time);
    } else {
        _extraLongEncode++;
        _averageExtraLongEncodeTime.updateAverage(time);
    }
    _averageEncodeTime.updateAverage(time);
}

void OctreeServer::trackTreeWaitTime(float time) {
    const float MAX_SHORT_TIME = 10.0f;
    const float MAX_LONG_TIME = 100.0f;
    if (time == SKIP_TIME) {
        _noTreeWait++;
        time = 0.0f;
    } else if (time <= MAX_SHORT_TIME) {
        _shortTreeWait++;
        _averageTreeShortWaitTime.updateAverage(time);
    } else if (time <= MAX_LONG_TIME) {
        _longTreeWait++;
        _averageTreeLongWaitTime.updateAverage(time);
    } else {
        _extraLongTreeWait++;
        _averageTreeExtraLongWaitTime.updateAverage(time);
    }
    _averageTreeWaitTime.updateAverage(time);
}

void OctreeServer::trackCompressAndWriteTime(float time) {
    const float MAX_SHORT_TIME = 10.0f;
    const float MAX_LONG_TIME = 100.0f;
    if (time == SKIP_TIME) {
        _noCompress++;
        time = 0.0f;
    } else if (time <= MAX_SHORT_TIME) {
        _shortCompress++;
        _averageShortCompressTime.updateAverage(time);
    } else if (time <= MAX_LONG_TIME) {
        _longCompress++;
        _averageLongCompressTime.updateAverage(time);
    } else {
        _extraLongCompress++;
        _averageExtraLongCompressTime.updateAverage(time);
    }
    _averageCompressAndWriteTime.updateAverage(time);
}

void OctreeServer::trackPacketSendingTime(float time) {
    if (time == SKIP_TIME) {
        _noSend++;
        time = 0.0f;
    }
    _averagePacketSendingTime.updateAverage(time);
}


void OctreeServer::trackProcessWaitTime(float time) {
    const float MAX_SHORT_TIME = 10.0f;
    const float MAX_LONG_TIME = 100.0f;
    if (time == SKIP_TIME) {
        _noProcessWait++;
        time = 0.0f;
    } else if (time <= MAX_SHORT_TIME) {
        _shortProcessWait++;
        _averageProcessShortWaitTime.updateAverage(time);
    } else if (time <= MAX_LONG_TIME) {
        _longProcessWait++;
        _averageProcessLongWaitTime.updateAverage(time);
    } else {
        _extraLongProcessWait++;
        _averageProcessExtraLongWaitTime.updateAverage(time);
    }
    _averageProcessWaitTime.updateAverage(time);
}

OctreeServer::OctreeServer(NLPacket& packet) :
    ThreadedAssignment(packet),
    _argc(0),
    _argv(NULL),
    _parsedArgV(NULL),
    _httpManager(NULL),
    _statusPort(0),
    _packetsPerClientPerInterval(10),
    _packetsTotalPerInterval(DEFAULT_PACKETS_PER_INTERVAL),
    _tree(NULL),
    _wantPersist(true),
    _debugSending(false),
    _debugReceiving(false),
    _verboseDebug(false),
    _jurisdiction(NULL),
    _jurisdictionSender(NULL),
    _octreeInboundPacketProcessor(NULL),
    _persistThread(NULL),
    _started(time(0)),
    _startedUSecs(usecTimestampNow())
{
    if (_instance) {
        qDebug() << "Octree Server starting... while old instance still running _instance=["<<_instance<<"] this=[" << this << "]";
    }

    qDebug() << "Octree Server starting... setting _instance to=[" << this << "]";
    _instance = this;

    _averageLoopTime.updateAverage(0);
    qDebug() << "Octree server starting... [" << this << "]";

    // make sure the AccountManager has an Auth URL for payment redemptions

    AccountManager::getInstance().setAuthURL(NetworkingConstants::METAVERSE_SERVER_URL);
}

OctreeServer::~OctreeServer() {
    qDebug() << qPrintable(_safeServerName) << "server shutting down... [" << this << "]";
    if (_parsedArgV) {
        for (int i = 0; i < _argc; i++) {
            delete[] _parsedArgV[i];
        }
        delete[] _parsedArgV;
    }

    if (_jurisdictionSender) {
        _jurisdictionSender->terminating();
        _jurisdictionSender->terminate();
        _jurisdictionSender->deleteLater();
    }

    if (_octreeInboundPacketProcessor) {
        _octreeInboundPacketProcessor->terminating();
        _octreeInboundPacketProcessor->terminate();
        _octreeInboundPacketProcessor->deleteLater();
    }

    if (_persistThread) {
        _persistThread->terminating();
        _persistThread->terminate();
        _persistThread->deleteLater();
    }

    delete _jurisdiction;
    _jurisdiction = NULL;

    // cleanup our tree here...
    qDebug() << qPrintable(_safeServerName) << "server START cleaning up octree... [" << this << "]";
    delete _tree;
    _tree = NULL;
    qDebug() << qPrintable(_safeServerName) << "server DONE cleaning up octree... [" << this << "]";

    if (_instance == this) {
        _instance = NULL; // we are gone
    }
    qDebug() << qPrintable(_safeServerName) << "server DONE shutting down... [" << this << "]";
}

void OctreeServer::initHTTPManager(int port) {
    // setup the embedded web server

    QString documentRoot = QString("%1/resources/web").arg(QCoreApplication::applicationDirPath());

    // setup an httpManager with us as the request handler and the parent
    _httpManager = new HTTPManager(port, documentRoot, this, this);
}

bool OctreeServer::handleHTTPRequest(HTTPConnection* connection, const QUrl& url, bool skipSubHandler) {

#ifdef FORCE_CRASH
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation
        && path == "/force_crash") {

        qDebug() << "About to force a crash!";

        int foo;
        int* forceCrash = &foo;

        QString responseString("forcing a crash...");
        connection->respond(HTTPConnection::StatusCode200, qPrintable(responseString));

        delete[] forceCrash;

        return true;
    }
#endif

    bool showStats = false;

    if (connection->requestOperation() == QNetworkAccessManager::GetOperation) {
        if (url.path() == "/") {
            showStats = true;
        } else if (url.path() == "/resetStats") {
            _octreeInboundPacketProcessor->resetStats();
            _tree->resetEditStats();
            resetSendingStats();
            showStats = true;
        }
    }

    if (showStats) {
        quint64 checkSum;
        // return a 200
        QString statsString("<html><doc>\r\n<pre>\r\n");
        statsString += QString("<b>Your %1 Server is running... <a href='/'>[RELOAD]</a></b>\r\n").arg(getMyServerName());

        tm* localtm = localtime(&_started);
        const int MAX_TIME_LENGTH = 128;
        char buffer[MAX_TIME_LENGTH];
        strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", localtm);
        statsString += QString("Running since: %1").arg(buffer);

        // Convert now to tm struct for UTC
        tm* gmtm = gmtime(&_started);
        if (gmtm) {
            strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", gmtm);
            statsString += (QString(" [%1 UTM] ").arg(buffer));
        }

        statsString += "\r\n";

        statsString += "Uptime: " + getUptime();
        statsString += "\r\n\r\n";

        // display octree file load time
        if (isInitialLoadComplete()) {
            if (isPersistEnabled()) {
                statsString += QString("%1 File Persist Enabled...\r\n").arg(getMyServerName());
            } else {
                statsString += QString("%1 File Persist Disabled...\r\n").arg(getMyServerName());
            }

            statsString += "\r\n";

            statsString += QString("%1 File Load Took ").arg(getMyServerName());
            statsString += getFileLoadTime();
            statsString += "\r\n";

        } else {
            statsString += "Octree file not yet loaded...\r\n";
        }

        statsString += "\r\n\r\n";
        statsString += "<b>Configuration:</b>\r\n";
        statsString += getConfiguration() + "\r\n"; //one to end the config line
        statsString += "\r\n";

        const int COLUMN_WIDTH = 19;
        QLocale locale(QLocale::English);
        const float AS_PERCENT = 100.0;

        statsString += QString("        Configured Max PPS/Client: %1 pps/client\r\n")
            .arg(locale.toString((uint)getPacketsPerClientPerSecond()).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("        Configured Max PPS/Server: %1 pps/server\r\n\r\n")
            .arg(locale.toString((uint)getPacketsTotalPerSecond()).rightJustified(COLUMN_WIDTH, ' '));


        // display scene stats
        unsigned long nodeCount = OctreeElement::getNodeCount();
        unsigned long internalNodeCount = OctreeElement::getInternalNodeCount();
        unsigned long leafNodeCount = OctreeElement::getLeafNodeCount();

        statsString += "<b>Current Elements in scene:</b>\r\n";
        statsString += QString("       Total Elements: %1 nodes\r\n")
            .arg(locale.toString((uint)nodeCount).rightJustified(16, ' '));
        statsString += QString().sprintf("    Internal Elements: %s nodes (%5.2f%%)\r\n",
                                         locale.toString((uint)internalNodeCount).rightJustified(16,
                                                                                                 ' ').toLocal8Bit().constData(),
                                         (double)((internalNodeCount / (float)nodeCount) * AS_PERCENT));
        statsString += QString().sprintf("        Leaf Elements: %s nodes (%5.2f%%)\r\n",
                                         locale.toString((uint)leafNodeCount).rightJustified(16, ' ').toLocal8Bit().constData(),
                                         (double)((leafNodeCount / (float)nodeCount) * AS_PERCENT));
        statsString += "\r\n";
        statsString += "\r\n";

        // display outbound packet stats
        statsString += QString("<b>%1 Outbound Packet Statistics... "
                                "<a href='/resetStats'>[RESET]</a></b>\r\n").arg(getMyServerName());

        quint64 totalOutboundPackets = OctreeSendThread::_totalPackets;
        quint64 totalOutboundBytes = OctreeSendThread::_totalBytes;
        quint64 totalWastedBytes = OctreeSendThread::_totalWastedBytes;
        quint64 totalBytesOfOctalCodes = OctreePacketData::getTotalBytesOfOctalCodes();
        quint64 totalBytesOfBitMasks = OctreePacketData::getTotalBytesOfBitMasks();
        quint64 totalBytesOfColor = OctreePacketData::getTotalBytesOfColor();

        statsString += QString("          Total Clients Connected: %1 clients\r\n")
            .arg(locale.toString((uint)getCurrentClientCount()).rightJustified(COLUMN_WIDTH, ' '));

        quint64 oneSecondAgo = usecTimestampNow() - USECS_PER_SECOND;

        statsString += QString("            process() last second: %1 clients\r\n")
            .arg(locale.toString((uint)howManyThreadsDidProcess(oneSecondAgo)).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("  packetDistributor() last second: %1 clients\r\n")
            .arg(locale.toString((uint)howManyThreadsDidPacketDistributor(oneSecondAgo)).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("   handlePacketSend() last second: %1 clients\r\n")
            .arg(locale.toString((uint)howManyThreadsDidHandlePacketSend(oneSecondAgo)).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("      writeDatagram() last second: %1 clients\r\n\r\n")
            .arg(locale.toString((uint)howManyThreadsDidCallWriteDatagram(oneSecondAgo)).rightJustified(COLUMN_WIDTH, ' '));

        float averageLoopTime = getAverageLoopTime();
        statsString += QString().sprintf("           Average packetLoop() time:      %7.2f msecs"
                                         "                 samples: %12d \r\n",
                                         (double)averageLoopTime, _averageLoopTime.getSampleCount());

        float averageInsideTime = getAverageInsideTime();
        statsString += QString().sprintf("               Average 'inside' time:    %9.2f usecs"
                                         "                 samples: %12d \r\n\r\n",
                                         (double)averageInsideTime, _averageInsideTime.getSampleCount());


        // Process Wait
        {
            int allWaitTimes = _extraLongProcessWait +_longProcessWait + _shortProcessWait + _noProcessWait;

            float averageProcessWaitTime = getAverageProcessWaitTime();
            statsString += QString().sprintf("      Average process lock wait time:"
                                             "    %9.2f usecs                 samples: %12d \r\n",
                                             (double)averageProcessWaitTime, allWaitTimes);

            float zeroVsTotal = (allWaitTimes > 0) ? ((float)_noProcessWait / (float)allWaitTimes) : 0.0f;
            statsString += QString().sprintf("                        No Lock Wait:"
                                             "                          (%6.2f%%) samples: %12d \r\n",
                                             (double)(zeroVsTotal * AS_PERCENT), _noProcessWait);

            float shortVsTotal = (allWaitTimes > 0) ? ((float)_shortProcessWait / (float)allWaitTimes) : 0.0f;
            statsString += QString().sprintf("    Avg process lock short wait time:"
                                             "          %9.2f usecs (%6.2f%%) samples: %12d \r\n",
                                             (double)_averageProcessShortWaitTime.getAverage(),
                                             (double)(shortVsTotal * AS_PERCENT), _shortProcessWait);

            float longVsTotal = (allWaitTimes > 0) ? ((float)_longProcessWait / (float)allWaitTimes) : 0.0f;
            statsString += QString().sprintf("     Avg process lock long wait time:"
                                             "          %9.2f usecs (%6.2f%%) samples: %12d \r\n",
                                             (double)_averageProcessLongWaitTime.getAverage(),
                                             (double)(longVsTotal * AS_PERCENT), _longProcessWait);

            float extraLongVsTotal = (allWaitTimes > 0) ? ((float)_extraLongProcessWait / (float)allWaitTimes) : 0.0f;
            statsString += QString().sprintf("Avg process lock extralong wait time:"
                                             "          %9.2f usecs (%6.2f%%) samples: %12d \r\n\r\n",
                                             (double)_averageProcessExtraLongWaitTime.getAverage(),
                                             (double)(extraLongVsTotal * AS_PERCENT), _extraLongProcessWait);
        }

        // Tree Wait
        int allWaitTimes = _extraLongTreeWait +_longTreeWait + _shortTreeWait + _noTreeWait;

        float averageTreeWaitTime = getAverageTreeWaitTime();
        statsString += QString().sprintf("         Average tree lock wait time:"
                                         "    %9.2f usecs                 samples: %12d \r\n",
                                         (double)averageTreeWaitTime, allWaitTimes);

        float zeroVsTotal = (allWaitTimes > 0) ? ((float)_noTreeWait / (float)allWaitTimes) : 0.0f;
        statsString += QString().sprintf("                        No Lock Wait:"
                                         "                          (%6.2f%%) samples: %12d \r\n",
                                         (double)(zeroVsTotal * AS_PERCENT), _noTreeWait);

        float shortVsTotal = (allWaitTimes > 0) ? ((float)_shortTreeWait / (float)allWaitTimes) : 0.0f;
        statsString += QString().sprintf("       Avg tree lock short wait time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n",
                                         (double)_averageTreeShortWaitTime.getAverage(),
                                         (double)(shortVsTotal * AS_PERCENT), _shortTreeWait);

        float longVsTotal = (allWaitTimes > 0) ? ((float)_longTreeWait / (float)allWaitTimes) : 0.0f;
        statsString += QString().sprintf("        Avg tree lock long wait time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n",
                                         (double)_averageTreeLongWaitTime.getAverage(),
                                         (double)(longVsTotal * AS_PERCENT), _longTreeWait);

        float extraLongVsTotal = (allWaitTimes > 0) ? ((float)_extraLongTreeWait / (float)allWaitTimes) : 0.0f;
        statsString += QString().sprintf("  Avg tree lock extra long wait time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n\r\n",
                                         (double)_averageTreeExtraLongWaitTime.getAverage(),
                                         (double)(extraLongVsTotal * AS_PERCENT), _extraLongTreeWait);

        // encode
        float averageEncodeTime = getAverageEncodeTime();
        statsString += QString().sprintf("                 Average encode time:    %9.2f usecs\r\n", (double)averageEncodeTime);

        int allEncodeTimes = _noEncode + _shortEncode + _longEncode + _extraLongEncode;

        float zeroVsTotalEncode = (allEncodeTimes > 0) ? ((float)_noEncode / (float)allEncodeTimes) : 0.0f;
        statsString += QString().sprintf("                           No Encode:"
                                         "                          (%6.2f%%) samples: %12d \r\n",
                                         (double)(zeroVsTotalEncode * AS_PERCENT), _noEncode);

        float shortVsTotalEncode = (allEncodeTimes > 0) ? ((float)_shortEncode / (float)allEncodeTimes) : 0.0f;
        statsString += QString().sprintf("               Avg short encode time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n",
                                         (double)_averageShortEncodeTime.getAverage(),
                                         (double)(shortVsTotalEncode * AS_PERCENT), _shortEncode);

        float longVsTotalEncode = (allEncodeTimes > 0) ? ((float)_longEncode / (float)allEncodeTimes) : 0.0f;
        statsString += QString().sprintf("                Avg long encode time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n",
                                         (double)_averageLongEncodeTime.getAverage(),
                                         (double)(longVsTotalEncode * AS_PERCENT), _longEncode);

        float extraLongVsTotalEncode = (allEncodeTimes > 0) ? ((float)_extraLongEncode / (float)allEncodeTimes) : 0.0f;
        statsString += QString().sprintf("          Avg extra long encode time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n\r\n",
                                         (double)_averageExtraLongEncodeTime.getAverage(),
                                         (double)(extraLongVsTotalEncode * AS_PERCENT), _extraLongEncode);


        float averageCompressAndWriteTime = getAverageCompressAndWriteTime();
        statsString += QString().sprintf("     Average compress and write time:    %9.2f usecs\r\n",
                                         (double)averageCompressAndWriteTime);

        int allCompressTimes = _noCompress + _shortCompress + _longCompress + _extraLongCompress;

        float zeroVsTotalCompress = (allCompressTimes > 0) ? ((float)_noCompress / (float)allCompressTimes) : 0.0f;
        statsString += QString().sprintf("                      No compression:"
                                         "                          (%6.2f%%) samples: %12d \r\n",
                                         (double)(zeroVsTotalCompress * AS_PERCENT), _noCompress);

        float shortVsTotalCompress = (allCompressTimes > 0) ? ((float)_shortCompress / (float)allCompressTimes) : 0.0f;
        statsString += QString().sprintf("             Avg short compress time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n",
                                         (double)_averageShortCompressTime.getAverage(),
                                         (double)(shortVsTotalCompress * AS_PERCENT), _shortCompress);

        float longVsTotalCompress = (allCompressTimes > 0) ? ((float)_longCompress / (float)allCompressTimes) : 0.0f;
        statsString += QString().sprintf("              Avg long compress time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n",
                                         (double)_averageLongCompressTime.getAverage(),
                                         (double)(longVsTotalCompress * AS_PERCENT), _longCompress);

        float extraLongVsTotalCompress = (allCompressTimes > 0) ? ((float)_extraLongCompress / (float)allCompressTimes) : 0.0f;
        statsString += QString().sprintf("        Avg extra long compress time:"
                                         "          %9.2f usecs (%6.2f%%) samples: %12d \r\n\r\n",
                                         (double)_averageExtraLongCompressTime.getAverage(),
                                         (double)(extraLongVsTotalCompress * AS_PERCENT), _extraLongCompress);

        float averagePacketSendingTime = getAveragePacketSendingTime();
        statsString += QString().sprintf("         Average packet sending time:    %9.2f usecs (includes node lock)\r\n",
                                         (double)averagePacketSendingTime);

        float noVsTotalSend = (_averagePacketSendingTime.getSampleCount() > 0) ?
                                        ((float)_noSend / (float)_averagePacketSendingTime.getSampleCount()) : 0.0f;
        statsString += QString().sprintf("                         Not sending:"
                                         "                          (%6.2f%%) samples: %12d \r\n",
                                         (double)(noVsTotalSend * AS_PERCENT), _noSend);

        float averageNodeWaitTime = getAverageNodeWaitTime();
        statsString += QString().sprintf("         Average node lock wait time:    %9.2f usecs\r\n",
                                         (double)averageNodeWaitTime);

        statsString += QString().sprintf("--------------------------------------------------------------\r\n");

        float encodeToInsidePercent = averageInsideTime == 0.0f ? 0.0f : (averageEncodeTime / averageInsideTime) * AS_PERCENT;
        statsString += QString().sprintf("                          encode ratio:      %5.2f%%\r\n",
                                         (double)encodeToInsidePercent);

        float waitToInsidePercent = averageInsideTime == 0.0f ? 0.0f
                    : ((averageTreeWaitTime + averageNodeWaitTime) / averageInsideTime) * AS_PERCENT;
        statsString += QString().sprintf("                         waiting ratio:      %5.2f%%\r\n",
                                         (double)waitToInsidePercent);

        float compressAndWriteToInsidePercent = averageInsideTime == 0.0f ? 0.0f
                    : (averageCompressAndWriteTime / averageInsideTime) * AS_PERCENT;
        statsString += QString().sprintf("              compress and write ratio:      %5.2f%%\r\n",
                                         (double)compressAndWriteToInsidePercent);

        float sendingToInsidePercent = averageInsideTime == 0.0f ? 0.0f
                    : (averagePacketSendingTime / averageInsideTime) * AS_PERCENT;
        statsString += QString().sprintf("                         sending ratio:      %5.2f%%\r\n",
                                         (double)sendingToInsidePercent);



        statsString += QString("\r\n");

        statsString += QString("           Total Outbound Packets: %1 packets\r\n")
            .arg(locale.toString((uint)totalOutboundPackets).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("             Total Outbound Bytes: %1 bytes\r\n")
            .arg(locale.toString((uint)totalOutboundBytes).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("               Total Wasted Bytes: %1 bytes\r\n")
            .arg(locale.toString((uint)totalWastedBytes).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString().sprintf("            Total OctalCode Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfOctalCodes).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            (double)((totalBytesOfOctalCodes / (float)totalOutboundBytes) * AS_PERCENT));
        statsString += QString().sprintf("             Total BitMasks Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfBitMasks).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            (double)(((float)totalBytesOfBitMasks / (float)totalOutboundBytes) * AS_PERCENT));
        statsString += QString().sprintf("                Total Color Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfColor).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            (double)((totalBytesOfColor / (float)totalOutboundBytes) * AS_PERCENT));

        statsString += "\r\n";
        statsString += "\r\n";

        // display inbound packet stats
        statsString += QString().sprintf("<b>%s Edit Statistics... <a href='/resetStats'>[RESET]</a></b>\r\n",
                                         getMyServerName());
        quint64 currentPacketsInQueue = _octreeInboundPacketProcessor->packetsToProcessCount();
        quint64 averageTransitTimePerPacket = _octreeInboundPacketProcessor->getAverageTransitTimePerPacket();
        quint64 averageProcessTimePerPacket = _octreeInboundPacketProcessor->getAverageProcessTimePerPacket();
        quint64 averageLockWaitTimePerPacket = _octreeInboundPacketProcessor->getAverageLockWaitTimePerPacket();
        quint64 averageProcessTimePerElement = _octreeInboundPacketProcessor->getAverageProcessTimePerElement();
        quint64 averageLockWaitTimePerElement = _octreeInboundPacketProcessor->getAverageLockWaitTimePerElement();
        quint64 totalElementsProcessed = _octreeInboundPacketProcessor->getTotalElementsProcessed();
        quint64 totalPacketsProcessed = _octreeInboundPacketProcessor->getTotalPacketsProcessed();

        quint64 averageDecodeTime = _tree->getAverageDecodeTime();
        quint64 averageLookupTime = _tree->getAverageLookupTime();
        quint64 averageUpdateTime = _tree->getAverageUpdateTime();
        quint64 averageCreateTime = _tree->getAverageCreateTime();
        quint64 averageLoggingTime = _tree->getAverageLoggingTime();


        float averageElementsPerPacket = totalPacketsProcessed == 0 ? 0 : totalElementsProcessed / totalPacketsProcessed;

        statsString += QString("   Current Inbound Packets Queue: %1 packets\r\n")
            .arg(locale.toString((uint)currentPacketsInQueue).rightJustified(COLUMN_WIDTH, ' '));

        statsString += QString("           Total Inbound Packets: %1 packets\r\n")
            .arg(locale.toString((uint)totalPacketsProcessed).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("          Total Inbound Elements: %1 elements\r\n")
            .arg(locale.toString((uint)totalElementsProcessed).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString().sprintf(" Average Inbound Elements/Packet: %f elements/packet\r\n",
                                         (double)averageElementsPerPacket);
        statsString += QString("     Average Transit Time/Packet: %1 usecs\r\n")
            .arg(locale.toString((uint)averageTransitTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("     Average Process Time/Packet: %1 usecs\r\n")
            .arg(locale.toString((uint)averageProcessTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("   Average Wait Lock Time/Packet: %1 usecs\r\n")
            .arg(locale.toString((uint)averageLockWaitTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("    Average Process Time/Element: %1 usecs\r\n")
            .arg(locale.toString((uint)averageProcessTimePerElement).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("  Average Wait Lock Time/Element: %1 usecs\r\n")
            .arg(locale.toString((uint)averageLockWaitTimePerElement).rightJustified(COLUMN_WIDTH, ' '));

        statsString += QString("             Average Decode Time: %1 usecs\r\n")
            .arg(locale.toString((uint)averageDecodeTime).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("             Average Lookup Time: %1 usecs\r\n")
            .arg(locale.toString((uint)averageLookupTime).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("             Average Update Time: %1 usecs\r\n")
            .arg(locale.toString((uint)averageUpdateTime).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("             Average Create Time: %1 usecs\r\n")
            .arg(locale.toString((uint)averageCreateTime).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("            Average Logging Time: %1 usecs\r\n")
            .arg(locale.toString((uint)averageLoggingTime).rightJustified(COLUMN_WIDTH, ' '));


        int senderNumber = 0;
        NodeToSenderStatsMap& allSenderStats = _octreeInboundPacketProcessor->getSingleSenderStats();
        for (NodeToSenderStatsMapConstIterator i = allSenderStats.begin(); i != allSenderStats.end(); i++) {
            senderNumber++;
            QUuid senderID = i.key();
            const SingleSenderStats& senderStats = i.value();

            statsString += QString("\r\n             Stats for sender %1 uuid: %2\r\n")
                .arg(senderNumber).arg(senderID.toString());

            averageTransitTimePerPacket = senderStats.getAverageTransitTimePerPacket();
            averageProcessTimePerPacket = senderStats.getAverageProcessTimePerPacket();
            averageLockWaitTimePerPacket = senderStats.getAverageLockWaitTimePerPacket();
            averageProcessTimePerElement = senderStats.getAverageProcessTimePerElement();
            averageLockWaitTimePerElement = senderStats.getAverageLockWaitTimePerElement();
            totalElementsProcessed = senderStats.getTotalElementsProcessed();
            totalPacketsProcessed = senderStats.getTotalPacketsProcessed();

            averageElementsPerPacket = totalPacketsProcessed == 0 ? 0 : totalElementsProcessed / totalPacketsProcessed;

            statsString += QString("               Total Inbound Packets: %1 packets\r\n")
                .arg(locale.toString((uint)totalPacketsProcessed).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("              Total Inbound Elements: %1 elements\r\n")
                .arg(locale.toString((uint)totalElementsProcessed).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString().sprintf("     Average Inbound Elements/Packet: %f elements/packet\r\n",
                                             (double)averageElementsPerPacket);
            statsString += QString("         Average Transit Time/Packet: %1 usecs\r\n")
                .arg(locale.toString((uint)averageTransitTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("        Average Process Time/Packet: %1 usecs\r\n")
                .arg(locale.toString((uint)averageProcessTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("       Average Wait Lock Time/Packet: %1 usecs\r\n")
                .arg(locale.toString((uint)averageLockWaitTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("        Average Process Time/Element: %1 usecs\r\n")
                .arg(locale.toString((uint)averageProcessTimePerElement).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("      Average Wait Lock Time/Element: %1 usecs\r\n")
                .arg(locale.toString((uint)averageLockWaitTimePerElement).rightJustified(COLUMN_WIDTH, ' '));

        }

        statsString += "\r\n\r\n";

        // display memory usage stats
        statsString += "<b>Current Memory Usage Statistics</b>\r\n";
        statsString += QString().sprintf("\r\nOctreeElement size... %ld bytes\r\n", sizeof(OctreeElement));
        statsString += "\r\n";

        const char* memoryScaleLabel;
        const float MEGABYTES = 1000000.0f;
        const float GIGABYTES = 1000000000.0f;
        float memoryScale;
        if (OctreeElement::getTotalMemoryUsage() / MEGABYTES < 1000.0f) {
            memoryScaleLabel = "MB";
            memoryScale = MEGABYTES;
        } else {
            memoryScaleLabel = "GB";
            memoryScale = GIGABYTES;
        }

        statsString += QString().sprintf("Element Node Memory Usage:       %8.2f %s\r\n",
                                         OctreeElement::getOctreeMemoryUsage() / (double)memoryScale, memoryScaleLabel);
        statsString += QString().sprintf("Octcode Memory Usage:            %8.2f %s\r\n",
                                         OctreeElement::getOctcodeMemoryUsage() / (double)memoryScale, memoryScaleLabel);
        statsString += QString().sprintf("External Children Memory Usage:  %8.2f %s\r\n",
                                         OctreeElement::getExternalChildrenMemoryUsage() / (double)memoryScale,
                                         memoryScaleLabel);
        statsString += "                                 -----------\r\n";
        statsString += QString().sprintf("                         Total:  %8.2f %s\r\n",
                                         OctreeElement::getTotalMemoryUsage() / (double)memoryScale, memoryScaleLabel);
        statsString += "\r\n";

        statsString += "OctreeElement Children Population Statistics...\r\n";
        checkSum = 0;
        for (int i=0; i <= NUMBER_OF_CHILDREN; i++) {
            checkSum += OctreeElement::getChildrenCount(i);
            statsString += QString().sprintf("    Nodes with %d children:      %s nodes (%5.2f%%)\r\n", i,
                locale.toString((uint)OctreeElement::getChildrenCount(i)).rightJustified(16, ' ').toLocal8Bit().constData(),
                (double)(((float)OctreeElement::getChildrenCount(i) / (float)nodeCount) * AS_PERCENT));
        }
        statsString += "                                ----------------------\r\n";
        statsString += QString("                    Total:      %1 nodes\r\n")
            .arg(locale.toString((uint)checkSum).rightJustified(16, ' '));

        statsString += "\r\n\r\n";
        statsString += "</pre>\r\n";
        statsString += "</doc></html>";

        connection->respond(HTTPConnection::StatusCode200, qPrintable(statsString), "text/html");

        return true;
    } else {
        // have HTTPManager attempt to process this request from the document_root
        return false;
    }

}

void OctreeServer::setArguments(int argc, char** argv) {
    _argc = argc;
    _argv = const_cast<const char**>(argv);

    qDebug("OctreeServer::setArguments()");
    for (int i = 0; i < _argc; i++) {
        qDebug("_argv[%d]=%s", i, _argv[i]);
    }

}

void OctreeServer::parsePayload() {

    if (getPayload().size() > 0) {
        QString config(_payload);

        // Now, parse the config
        QStringList configList = config.split(" ");

        int argCount = configList.size() + 1;

        qDebug("OctreeServer::parsePayload()... argCount=%d",argCount);

        _parsedArgV = new char*[argCount];
        const char* dummy = "config-from-payload";
        _parsedArgV[0] = new char[strlen(dummy) + sizeof(char)];
        strcpy(_parsedArgV[0], dummy);

        for (int i = 1; i < argCount; i++) {
            QString configItem = configList.at(i-1);
            _parsedArgV[i] = new char[configItem.length() + sizeof(char)];
            strcpy(_parsedArgV[i], configItem.toLocal8Bit().constData());
            qDebug("OctreeServer::parsePayload()... _parsedArgV[%d]=%s", i, _parsedArgV[i]);
        }

        setArguments(argCount, _parsedArgV);
    }
}

void OctreeServer::handleOctreeQueryPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    if (!_isFinished) {
        // If we got a query packet, then we're talking to an agent, and we
        // need to make sure we have it in our nodeList.
        auto nodeList = DependencyManager::get<NodeList>();
        nodeList->updateNodeWithDataFromPacket(packet, senderNode);
        
        OctreeQueryNode* nodeData = dynamic_cast<OctreeQueryNode*>(senderNode->getLinkedData());
        if (nodeData && !nodeData->isOctreeSendThreadInitalized()) {
            nodeData->initializeOctreeSendThread(this, senderNode);
        }
    }
}

void OctreeServer::handleOctreeDataNackPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    // If we got a nack packet, then we're talking to an agent, and we
    // need to make sure we have it in our nodeList.
    OctreeQueryNode* nodeData = dynamic_cast<OctreeQueryNode*>(senderNode->getLinkedData());
    if (nodeData) {
        nodeData->parseNackPacket(*packet);
    }
}

void OctreeServer::handleJurisdictionRequestPacket(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    _jurisdictionSender->queueReceivedPacket(packet, senderNode);
}

bool OctreeServer::readOptionBool(const QString& optionName, const QJsonObject& settingsSectionObject, bool& result) {
    result = false; // assume it doesn't exist
    bool optionAvailable = false;
    QString argName = "--" + optionName;
    bool argExists = cmdOptionExists(_argc, _argv, qPrintable(argName));
    if (argExists) {
        optionAvailable = true;
        result = argExists;
        qDebug() << "From payload arguments: " << qPrintable(argName) << ":" << result;
    } else if (settingsSectionObject.contains(optionName)) {
        optionAvailable = true;
        result = settingsSectionObject[optionName].toBool();
        qDebug() << "From domain settings: " << qPrintable(optionName) << ":" << result;
    }
    return optionAvailable;
}

bool OctreeServer::readOptionInt(const QString& optionName, const QJsonObject& settingsSectionObject, int& result) {
    bool optionAvailable = false;
    QString argName = "--" + optionName;
    const char* argValue = getCmdOption(_argc, _argv, qPrintable(argName));
    if (argValue) {
        optionAvailable = true;
        result = atoi(argValue);
        qDebug() << "From payload arguments: " << qPrintable(argName) << ":" << result;
    } else if (settingsSectionObject.contains(optionName)) {
        optionAvailable = true;
        result = settingsSectionObject[optionName].toString().toInt(&optionAvailable);
        if (optionAvailable) {
            qDebug() << "From domain settings: " << qPrintable(optionName) << ":" << result;
        }
    }
    return optionAvailable;
}

bool OctreeServer::readOptionString(const QString& optionName, const QJsonObject& settingsSectionObject, QString& result) {
    bool optionAvailable = false;
    QString argName = "--" + optionName;
    const char* argValue = getCmdOption(_argc, _argv, qPrintable(argName));
    if (argValue) {
        optionAvailable = true;
        result = QString(argValue);
        qDebug() << "From payload arguments: " << qPrintable(argName) << ":" << qPrintable(result);
    } else if (settingsSectionObject.contains(optionName)) {
        optionAvailable = true;
        result = settingsSectionObject[optionName].toString();
        qDebug() << "From domain settings: " << qPrintable(optionName) << ":" << qPrintable(result);
    }
    return optionAvailable;
}

void OctreeServer::readConfiguration() {
    // if the assignment had a payload, read and parse that
    if (getPayload().size() > 0) {
        parsePayload();
    }

    // wait until we have the domain-server settings, otherwise we bail
    auto nodeList = DependencyManager::get<NodeList>();
    DomainHandler& domainHandler = nodeList->getDomainHandler();

    qDebug() << "Waiting for domain settings from domain-server.";

    // block until we get the settingsRequestComplete signal
    QEventLoop loop;
    connect(&domainHandler, &DomainHandler::settingsReceived, &loop, &QEventLoop::quit);
    connect(&domainHandler, &DomainHandler::settingsReceiveFail, &loop, &QEventLoop::quit);
    domainHandler.requestDomainSettings();
    loop.exec();

    if (domainHandler.getSettingsObject().isEmpty()) {
        qDebug() << "No settings object from domain-server.";
    }
    const QJsonObject& settingsObject = domainHandler.getSettingsObject();
    QString settingsKey = getMyDomainSettingsKey();
    QJsonObject settingsSectionObject = settingsObject[settingsKey].toObject();
    _settings = settingsSectionObject; // keep this for later

    if (!readOptionString(QString("statusHost"), settingsSectionObject, _statusHost) || _statusHost.isEmpty()) {
        _statusHost = getLocalAddress().toString();
    }
    qDebug("statusHost=%s", qPrintable(_statusHost));

    if (readOptionInt(QString("statusPort"), settingsSectionObject, _statusPort)) {
        initHTTPManager(_statusPort);
        qDebug() << "statusPort=" << _statusPort;
    } else {
        qDebug() << "statusPort= DISABLED";
    }

    QString jurisdictionFile;
    if (readOptionString(QString("jurisdictionFile"), settingsSectionObject, jurisdictionFile)) {
        qDebug("jurisdictionFile=%s", qPrintable(jurisdictionFile));
        qDebug("about to readFromFile().... jurisdictionFile=%s", qPrintable(jurisdictionFile));
        _jurisdiction = new JurisdictionMap(qPrintable(jurisdictionFile));
        qDebug("after readFromFile().... jurisdictionFile=%s", qPrintable(jurisdictionFile));
    } else {
        QString jurisdictionRoot;
        bool hasRoot = readOptionString(QString("jurisdictionRoot"), settingsSectionObject, jurisdictionRoot);
        QString jurisdictionEndNodes;
        bool hasEndNodes = readOptionString(QString("jurisdictionEndNodes"), settingsSectionObject, jurisdictionEndNodes);

        if (hasRoot || hasEndNodes) {
            _jurisdiction = new JurisdictionMap(qPrintable(jurisdictionRoot), qPrintable(jurisdictionEndNodes));
        }
    }

    readOptionBool(QString("verboseDebug"), settingsSectionObject, _verboseDebug);
    qDebug("verboseDebug=%s", debug::valueOf(_verboseDebug));

    readOptionBool(QString("debugSending"), settingsSectionObject, _debugSending);
    qDebug("debugSending=%s", debug::valueOf(_debugSending));

    readOptionBool(QString("debugReceiving"), settingsSectionObject, _debugReceiving);
    qDebug("debugReceiving=%s", debug::valueOf(_debugReceiving));

    readOptionBool(QString("debugTimestampNow"), settingsSectionObject, _debugTimestampNow);
    qDebug() << "debugTimestampNow=" << _debugTimestampNow;

    bool noPersist;
    readOptionBool(QString("NoPersist"), settingsSectionObject, noPersist);
    _wantPersist = !noPersist;
    qDebug() << "wantPersist=" << _wantPersist;

    if (_wantPersist) {
        QString persistFilename;
        if (!readOptionString(QString("persistFilename"), settingsSectionObject, persistFilename)) {
            persistFilename = getMyDefaultPersistFilename();
        }
        strcpy(_persistFilename, qPrintable(persistFilename));
        qDebug("persistFilename=%s", _persistFilename);

        QString persistAsFileType;
        if (!readOptionString(QString("persistAsFileType"), settingsSectionObject, persistAsFileType)) {
            persistAsFileType = "svo";
        }
        _persistAsFileType = persistAsFileType;
        qDebug() << "persistAsFileType=" << _persistAsFileType;

        _persistInterval = OctreePersistThread::DEFAULT_PERSIST_INTERVAL;
        readOptionInt(QString("persistInterval"), settingsSectionObject, _persistInterval);
        qDebug() << "persistInterval=" << _persistInterval;

        bool noBackup;
        readOptionBool(QString("NoBackup"), settingsSectionObject, noBackup);
        _wantBackup = !noBackup;
        qDebug() << "wantBackup=" << _wantBackup;

        //qDebug() << "settingsSectionObject:" << settingsSectionObject;

    } else {
        qDebug("persistFilename= DISABLED");
    }

    // Debug option to demonstrate that the server's local time does not
    // need to be in sync with any other network node. This forces clock
    // skew for the individual server node
    int clockSkew;
    if (readOptionInt(QString("clockSkew"), settingsSectionObject, clockSkew)) {
        usecTimestampNowForceClockSkew(clockSkew);
        qDebug("clockSkew=%d", clockSkew);
    }

    // Check to see if the user passed in a command line option for setting packet send rate
    int packetsPerSecondPerClientMax = -1;
    if (readOptionInt(QString("packetsPerSecondPerClientMax"), settingsSectionObject, packetsPerSecondPerClientMax)) {
        _packetsPerClientPerInterval = packetsPerSecondPerClientMax / INTERVALS_PER_SECOND;
        if (_packetsPerClientPerInterval < 1) {
            _packetsPerClientPerInterval = 1;
        }
    }
    qDebug("packetsPerSecondPerClientMax=%d _packetsPerClientPerInterval=%d",
                    packetsPerSecondPerClientMax, _packetsPerClientPerInterval);

    // Check to see if the user passed in a command line option for setting packet send rate
    int packetsPerSecondTotalMax = -1;
    if (readOptionInt(QString("packetsPerSecondTotalMax"), settingsSectionObject, packetsPerSecondTotalMax)) {
        _packetsTotalPerInterval = packetsPerSecondTotalMax / INTERVALS_PER_SECOND;
        if (_packetsTotalPerInterval < 1) {
            _packetsTotalPerInterval = 1;
        }
    }
    qDebug("packetsPerSecondTotalMax=%d _packetsTotalPerInterval=%d",
                    packetsPerSecondTotalMax, _packetsTotalPerInterval);


    readAdditionalConfiguration(settingsSectionObject);
}

void OctreeServer::run() {

    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(getMyQueryMessageType(), this, "handleOctreeQueryPacket");
    packetReceiver.registerListener(PacketType::OctreeDataNack, this, "handleOctreeDataNackPacket");
    packetReceiver.registerListener(PacketType::JurisdictionRequest, this, "handleJurisdictionRequestPacket");
    
    _safeServerName = getMyServerName();

    // Before we do anything else, create our tree...
    OctreeElement::resetPopulationStatistics();
    _tree = createTree();
    _tree->setIsServer(true);

    // make sure our NodeList knows what type we are
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->setOwnerType(getMyNodeType());


    // use common init to setup common timers and logging
    commonInit(getMyLoggingServerTargetName(), getMyNodeType());

    // read the configuration from either the payload or the domain server configuration
    readConfiguration();

    beforeRun(); // after payload has been processed

    connect(nodeList.data(), SIGNAL(nodeAdded(SharedNodePointer)), SLOT(nodeAdded(SharedNodePointer)));
    connect(nodeList.data(), SIGNAL(nodeKilled(SharedNodePointer)), SLOT(nodeKilled(SharedNodePointer)));


    // we need to ask the DS about agents so we can ping/reply with them
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

#ifndef WIN32
    setvbuf(stdout, NULL, _IOLBF, 0);
#endif

    nodeList->linkedDataCreateCallback = [] (Node* node) {
        OctreeQueryNode* newQueryNodeData = _instance->createOctreeQueryNode();
        newQueryNodeData->init();
        node->setLinkedData(newQueryNodeData);
    };

    srand((unsigned)time(0));

    // if we want Persistence, set up the local file and persist thread
    if (_wantPersist) {

        // now set up PersistThread
        _persistThread = new OctreePersistThread(_tree, _persistFilename, _persistInterval,
                                                 _wantBackup, _settings, _debugTimestampNow, _persistAsFileType);
        if (_persistThread) {
            _persistThread->initialize(true);
        }
    }

    HifiSockAddr senderSockAddr;

    // set up our jurisdiction broadcaster...
    if (_jurisdiction) {
        _jurisdiction->setNodeType(getMyNodeType());
    }
    _jurisdictionSender = new JurisdictionSender(_jurisdiction, getMyNodeType());
    _jurisdictionSender->initialize(true);

    // set up our OctreeServerPacketProcessor
    _octreeInboundPacketProcessor = new OctreeInboundPacketProcessor(this);
    _octreeInboundPacketProcessor->initialize(true);

    // Convert now to tm struct for local timezone
    tm* localtm = localtime(&_started);
    const int MAX_TIME_LENGTH = 128;
    char localBuffer[MAX_TIME_LENGTH] = { 0 };
    char utcBuffer[MAX_TIME_LENGTH] = { 0 };
    strftime(localBuffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", localtm);
    // Convert now to tm struct for UTC
    tm* gmtm = gmtime(&_started);
    if (gmtm) {
        strftime(utcBuffer, MAX_TIME_LENGTH, " [%m/%d/%Y %X UTC]", gmtm);
    }
    qDebug() << "Now running... started at: " << localBuffer << utcBuffer;
}

void OctreeServer::nodeAdded(SharedNodePointer node) {
    // we might choose to use this notifier to track clients in a pending state
    qDebug() << qPrintable(_safeServerName) << "server added node:" << *node;
}

void OctreeServer::nodeKilled(SharedNodePointer node) {
    quint64 start  = usecTimestampNow();

    // calling this here since nodeKilled slot in ReceivedPacketProcessor can't be triggered by signals yet!!
    _octreeInboundPacketProcessor->nodeKilled(node);

    qDebug() << qPrintable(_safeServerName) << "server killed node:" << *node;
    OctreeQueryNode* nodeData = dynamic_cast<OctreeQueryNode*>(node->getLinkedData());
    if (nodeData) {
        nodeData->nodeKilled(); // tell our node data and sending threads that we'd like to shut down
    } else {
        qDebug() << qPrintable(_safeServerName) << "server node missing linked data node:" << *node;
    }

    quint64 end  = usecTimestampNow();
    quint64 usecsElapsed = (end - start);
    if (usecsElapsed > 1000) {
        qDebug() << qPrintable(_safeServerName) << "server nodeKilled() took: " << usecsElapsed << " usecs for node:" << *node;
    }
}

void OctreeServer::forceNodeShutdown(SharedNodePointer node) {
    quint64 start  = usecTimestampNow();

    qDebug() << qPrintable(_safeServerName) << "server killed node:" << *node;
    OctreeQueryNode* nodeData = dynamic_cast<OctreeQueryNode*>(node->getLinkedData());
    if (nodeData) {
        nodeData->forceNodeShutdown(); // tell our node data and sending threads that we'd like to shut down
    } else {
        qDebug() << qPrintable(_safeServerName) << "server node missing linked data node:" << *node;
    }

    quint64 end  = usecTimestampNow();
    quint64 usecsElapsed = (end - start);
    qDebug() << qPrintable(_safeServerName) << "server forceNodeShutdown() took: "
                << usecsElapsed << " usecs for node:" << *node;
}


void OctreeServer::aboutToFinish() {
    qDebug() << qPrintable(_safeServerName) << "server STARTING about to finish...";

    _isShuttingDown = true;

    qDebug() << qPrintable(_safeServerName) << "inform Octree Inbound Packet Processor that we are shutting down...";

    // we're going down - set the NodeList linkedDataCallback to NULL so we do not create any more OctreeQueryNode objects.
    // This ensures that when we forceNodeShutdown below for each node we don't get any more newly connecting nodes
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->linkedDataCreateCallback = NULL;

    if (_octreeInboundPacketProcessor) {
        _octreeInboundPacketProcessor->terminating();
    }

    if (_jurisdictionSender) {
        _jurisdictionSender->terminating();
    }

    // force a shutdown of all of our OctreeSendThreads - at this point it has to be impossible for a
    // linkedDataCreateCallback to be called for a new node
    nodeList->eachNode([this](const SharedNodePointer& node) {
        qDebug() << qPrintable(_safeServerName) << "server about to finish while node still connected node:" << *node;
        forceNodeShutdown(node);
    });

    if (_persistThread) {
        _persistThread->aboutToFinish();
        _persistThread->terminating();
    }

    qDebug() << qPrintable(_safeServerName) << "server ENDING about to finish...";
}

QString OctreeServer::getUptime() {
    QString formattedUptime;
    quint64 now  = usecTimestampNow();
    const int USECS_PER_MSEC = 1000;
    quint64 msecsElapsed = (now - _startedUSecs) / USECS_PER_MSEC;
    const int MSECS_PER_SEC = 1000;
    const int SECS_PER_MIN = 60;
    const int MIN_PER_HOUR = 60;
    const int MSECS_PER_MIN = MSECS_PER_SEC * SECS_PER_MIN;

    float seconds = (msecsElapsed % MSECS_PER_MIN)/(float)MSECS_PER_SEC;
    int minutes = (msecsElapsed/(MSECS_PER_MIN)) % MIN_PER_HOUR;
    int hours = (msecsElapsed/(MSECS_PER_MIN * MIN_PER_HOUR));

    if (hours > 0) {
        formattedUptime += QString("%1 hour").arg(hours);
        if (hours > 1) {
            formattedUptime += QString("s");
        }
    }
    if (minutes > 0) {
        if (hours > 0) {
            formattedUptime += QString(" ");
        }
        formattedUptime += QString("%1 minute").arg(minutes);
        if (minutes > 1) {
            formattedUptime += QString("s");
        }
    }
    if (seconds > 0) {
        if (hours > 0 || minutes > 0) {
            formattedUptime += QString(" ");
        }
        formattedUptime += QString().sprintf("%.3f seconds", (double)seconds);
    }
    return formattedUptime;
}

QString OctreeServer::getFileLoadTime() {
    QString result;
    if (isInitialLoadComplete()) {

        const int USECS_PER_MSEC = 1000;
        const int MSECS_PER_SEC = 1000;
        const int SECS_PER_MIN = 60;
        const int MIN_PER_HOUR = 60;
        const int MSECS_PER_MIN = MSECS_PER_SEC * SECS_PER_MIN;

        quint64 msecsElapsed = getLoadElapsedTime() / USECS_PER_MSEC;;
        float seconds = (msecsElapsed % MSECS_PER_MIN)/(float)MSECS_PER_SEC;
        int minutes = (msecsElapsed/(MSECS_PER_MIN)) % MIN_PER_HOUR;
        int hours = (msecsElapsed/(MSECS_PER_MIN * MIN_PER_HOUR));

        if (hours > 0) {
            result += QString("%1 hour").arg(hours);
            if (hours > 1) {
                result += QString("s");
            }
        }
        if (minutes > 0) {
            if (hours > 0) {
                result += QString(" ");
            }
            result += QString("%1 minute").arg(minutes);
            if (minutes > 1) {
                result += QString("s");
            }
        }
        if (seconds >= 0) {
            if (hours > 0 || minutes > 0) {
                result += QString(" ");
            }
            result += QString().sprintf("%.3f seconds", (double)seconds);
        }
    } else {
        result = "Not yet loaded...";
    }
    return result;
}

QString OctreeServer::getConfiguration() {
    QString result;
    for (int i = 1; i < _argc; i++) {
        result += _argv[i] + QString(" ");
    }
    return result;
}

QString OctreeServer::getStatusLink() {
    QString result;
    if (_statusPort > 0) {
        QString detailedStats= QString("http://") +  _statusHost + QString(":%1").arg(_statusPort);
        result = "<a href='" + detailedStats + "'>"+detailedStats+"</a>";
    } else {
        result = "Status port not enabled.";
    }
    return result;
}

void OctreeServer::sendStatsPacket() {
    // TODO: we have too many stats to fit in a single MTU... so for now, we break it into multiple JSON objects and
    // send them separately. What we really should do is change the NodeList::sendStatsToDomainServer() to handle the
    // the following features:
    //    1) remember last state sent
    //    2) only send new data
    //    3) automatically break up into multiple packets
    static QJsonObject statsObject1;

    QString baseName = getMyServerName() + QString("Server");

    statsObject1[baseName + QString(".0.1.configuration")] = getConfiguration();

    statsObject1[baseName + QString(".0.2.detailed_stats_url")] = getStatusLink();

    statsObject1[baseName + QString(".0.3.uptime")] = getUptime();
    statsObject1[baseName + QString(".0.4.persistFileLoadTime")] = getFileLoadTime();
    statsObject1[baseName + QString(".0.5.clients")] = getCurrentClientCount();

    quint64 oneSecondAgo = usecTimestampNow() - USECS_PER_SECOND;

    statsObject1[baseName + QString(".0.6.threads.1.processing")] = (double)howManyThreadsDidProcess(oneSecondAgo);
    statsObject1[baseName + QString(".0.6.threads.2.packetDistributor")] =
        (double)howManyThreadsDidPacketDistributor(oneSecondAgo);
    statsObject1[baseName + QString(".0.6.threads.3.handlePacektSend")] =
        (double)howManyThreadsDidHandlePacketSend(oneSecondAgo);
    statsObject1[baseName + QString(".0.6.threads.4.writeDatagram")] =
        (double)howManyThreadsDidCallWriteDatagram(oneSecondAgo);

    statsObject1[baseName + QString(".1.1.octree.elementCount")] = (double)OctreeElement::getNodeCount();
    statsObject1[baseName + QString(".1.2.octree.internalElementCount")] = (double)OctreeElement::getInternalNodeCount();
    statsObject1[baseName + QString(".1.3.octree.leafElementCount")] = (double)OctreeElement::getLeafNodeCount();

    ThreadedAssignment::addPacketStatsAndSendStatsPacket(statsObject1);

    static QJsonObject statsObject2;

    statsObject2[baseName + QString(".2.outbound.data.totalPackets")] = (double)OctreeSendThread::_totalPackets;
    statsObject2[baseName + QString(".2.outbound.data.totalBytes")] = (double)OctreeSendThread::_totalBytes;
    statsObject2[baseName + QString(".2.outbound.data.totalBytesWasted")] = (double)OctreeSendThread::_totalWastedBytes;
    statsObject2[baseName + QString(".2.outbound.data.totalBytesOctalCodes")] =
        (double)OctreePacketData::getTotalBytesOfOctalCodes();
    statsObject2[baseName + QString(".2.outbound.data.totalBytesBitMasks")] =
        (double)OctreePacketData::getTotalBytesOfBitMasks();
    statsObject2[baseName + QString(".2.outbound.data.totalBytesBitMasks")] = (double)OctreePacketData::getTotalBytesOfColor();

    statsObject2[baseName + QString(".2.outbound.timing.1.avgLoopTime")] = getAverageLoopTime();
    statsObject2[baseName + QString(".2.outbound.timing.2.avgInsideTime")] = getAverageInsideTime();
    statsObject2[baseName + QString(".2.outbound.timing.3.avgTreeLockTime")] = getAverageTreeWaitTime();
    statsObject2[baseName + QString(".2.outbound.timing.4.avgEncodeTime")] = getAverageEncodeTime();
    statsObject2[baseName + QString(".2.outbound.timing.5.avgCompressAndWriteTime")] = getAverageCompressAndWriteTime();
    statsObject2[baseName + QString(".2.outbound.timing.5.avgSendTime")] = getAveragePacketSendingTime();
    statsObject2[baseName + QString(".2.outbound.timing.5.nodeWaitTime")] = getAverageNodeWaitTime();

    DependencyManager::get<NodeList>()->sendStatsToDomainServer(statsObject2);

    static QJsonObject statsObject3;

    statsObject3[baseName + QString(".3.inbound.data.1.packetQueue")] =
        (double)_octreeInboundPacketProcessor->packetsToProcessCount();
    statsObject3[baseName + QString(".3.inbound.data.1.totalPackets")] =
        (double)_octreeInboundPacketProcessor->getTotalPacketsProcessed();
    statsObject3[baseName + QString(".3.inbound.data.2.totalElements")] =
        (double)_octreeInboundPacketProcessor->getTotalElementsProcessed();
    statsObject3[baseName + QString(".3.inbound.timing.1.avgTransitTimePerPacket")] =
        (double)_octreeInboundPacketProcessor->getAverageTransitTimePerPacket();
    statsObject3[baseName + QString(".3.inbound.timing.2.avgProcessTimePerPacket")] =
        (double)_octreeInboundPacketProcessor->getAverageProcessTimePerPacket();
    statsObject3[baseName + QString(".3.inbound.timing.3.avgLockWaitTimePerPacket")] =
        (double)_octreeInboundPacketProcessor->getAverageLockWaitTimePerPacket();
    statsObject3[baseName + QString(".3.inbound.timing.4.avgProcessTimePerElement")] =
        (double)_octreeInboundPacketProcessor->getAverageProcessTimePerElement();
    statsObject3[baseName + QString(".3.inbound.timing.5.avgLockWaitTimePerElement")] =
        (double)_octreeInboundPacketProcessor->getAverageLockWaitTimePerElement();

    DependencyManager::get<NodeList>()->sendStatsToDomainServer(statsObject3);
}

QMap<OctreeSendThread*, quint64> OctreeServer::_threadsDidProcess;
QMap<OctreeSendThread*, quint64> OctreeServer::_threadsDidPacketDistributor;
QMap<OctreeSendThread*, quint64> OctreeServer::_threadsDidHandlePacketSend;
QMap<OctreeSendThread*, quint64> OctreeServer::_threadsDidCallWriteDatagram;

QMutex OctreeServer::_threadsDidProcessMutex;
QMutex OctreeServer::_threadsDidPacketDistributorMutex;
QMutex OctreeServer::_threadsDidHandlePacketSendMutex;
QMutex OctreeServer::_threadsDidCallWriteDatagramMutex;


void OctreeServer::didProcess(OctreeSendThread* thread) {
    QMutexLocker locker(&_threadsDidProcessMutex);
    _threadsDidProcess[thread] = usecTimestampNow();
}

void OctreeServer::didPacketDistributor(OctreeSendThread* thread) {
    QMutexLocker locker(&_threadsDidPacketDistributorMutex);
    _threadsDidPacketDistributor[thread] = usecTimestampNow();
}

void OctreeServer::didHandlePacketSend(OctreeSendThread* thread) {
    QMutexLocker locker(&_threadsDidHandlePacketSendMutex);
    _threadsDidHandlePacketSend[thread] = usecTimestampNow();
}

void OctreeServer::didCallWriteDatagram(OctreeSendThread* thread) {
    QMutexLocker locker(&_threadsDidCallWriteDatagramMutex);
    _threadsDidCallWriteDatagram[thread] = usecTimestampNow();
}


void OctreeServer::stopTrackingThread(OctreeSendThread* thread) {
    QMutexLocker lockerA(&_threadsDidProcessMutex);
    QMutexLocker lockerB(&_threadsDidPacketDistributorMutex);
    QMutexLocker lockerC(&_threadsDidHandlePacketSendMutex);
    QMutexLocker lockerD(&_threadsDidCallWriteDatagramMutex);

    _threadsDidProcess.remove(thread);
    _threadsDidPacketDistributor.remove(thread);
    _threadsDidHandlePacketSend.remove(thread);
    _threadsDidCallWriteDatagram.remove(thread);
}

int howManyThreadsDidSomething(QMutex& mutex, QMap<OctreeSendThread*, quint64>& something, quint64 since) {
    int count = 0;
    if (mutex.tryLock()) {
        if (since == 0) {
            count = something.size();
        } else {
            QMap<OctreeSendThread*, quint64>::const_iterator i = something.constBegin();
            while (i != something.constEnd()) {
                if (i.value() > since) {
                    count++;
                }
                ++i;
            }
        }
        mutex.unlock();
    }
    return count;
}


int OctreeServer::howManyThreadsDidProcess(quint64 since) {
    return howManyThreadsDidSomething(_threadsDidProcessMutex, _threadsDidProcess, since);
}

int OctreeServer::howManyThreadsDidPacketDistributor(quint64 since) {
    return howManyThreadsDidSomething(_threadsDidPacketDistributorMutex, _threadsDidPacketDistributor, since);
}

int OctreeServer::howManyThreadsDidHandlePacketSend(quint64 since) {
    return howManyThreadsDidSomething(_threadsDidHandlePacketSendMutex, _threadsDidHandlePacketSend, since);
}

int OctreeServer::howManyThreadsDidCallWriteDatagram(quint64 since) {
    return howManyThreadsDidSomething(_threadsDidCallWriteDatagramMutex, _threadsDidCallWriteDatagram, since);
}

