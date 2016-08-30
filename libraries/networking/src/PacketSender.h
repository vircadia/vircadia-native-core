//
//  PacketSender.h
//  libraries/networking/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Threaded or non-threaded packet sender.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PacketSender_h
#define hifi_PacketSender_h

#include <QWaitCondition>

#include "GenericThread.h"
#include "NodeList.h"
#include "SharedUtil.h"

/// Generalized threaded processor for queueing and sending of outbound packets.
class PacketSender : public GenericThread {
    Q_OBJECT
public:

    static const quint64 USECS_PER_SECOND;
    static const quint64 SENDING_INTERVAL_ADJUST;
    static const int TARGET_FPS;
    static const int MAX_SLEEP_INTERVAL;

    static const int DEFAULT_PACKETS_PER_SECOND;
    static const int MINIMUM_PACKETS_PER_SECOND;
    static const int MINIMAL_SLEEP_INTERVAL;

    PacketSender(int packetsPerSecond = DEFAULT_PACKETS_PER_SECOND);
    ~PacketSender();

    /// Add packet to outbound queue.
    void queuePacketForSending(const SharedNodePointer& destinationNode, std::unique_ptr<NLPacket> packet);

    void setPacketsPerSecond(int packetsPerSecond);
    int getPacketsPerSecond() const { return _packetsPerSecond; }

    virtual bool process() override;
    virtual void terminating() override;

    /// are there packets waiting in the send queue to be sent
    bool hasPacketsToSend() const { return _packets.size() > 0; }

    /// how many packets are there in the send queue waiting to be sent
    size_t packetsToSendCount() const { return _packets.size(); }

    /// If you're running in non-threaded mode, call this to give us a hint as to how frequently you will call process.
    /// This has no effect in threaded mode. This is only considered a hint in non-threaded mode.
    /// \param int usecsPerProcessCall expected number of usecs between calls to process in non-threaded mode.
    void setProcessCallIntervalHint(int usecsPerProcessCall) { _usecsPerProcessCallHint = usecsPerProcessCall; }

    /// returns the packets per second send rate of this object over its lifetime
    float getLifetimePPS() const
        { return getLifetimeInSeconds() == 0 ? 0 : (float)((float)_totalPacketsSent / getLifetimeInSeconds()); }

    /// returns the bytes per second send rate of this object over its lifetime
    float getLifetimeBPS() const
        { return getLifetimeInSeconds() == 0 ? 0 : (float)((float)_totalBytesSent / getLifetimeInSeconds()); }

    /// returns the packets per second queued rate of this object over its lifetime
    float getLifetimePPSQueued() const
        { return getLifetimeInSeconds() == 0 ? 0 : (float)((float)_totalPacketsQueued / getLifetimeInSeconds()); }

    /// returns the bytes per second queued rate of this object over its lifetime
    float getLifetimeBPSQueued() const
        { return getLifetimeInSeconds() == 0 ? 0 : (float)((float)_totalBytesQueued / getLifetimeInSeconds()); }

    /// returns lifetime of this object from first packet sent to now in usecs
    quint64 getLifetimeInUsecs() const { return (usecTimestampNow() - _started); }

    /// returns lifetime of this object from first packet sent to now in usecs
    float getLifetimeInSeconds() const { return ((float)getLifetimeInUsecs() / (float)USECS_PER_SECOND); }

    /// returns the total packets sent by this object over its lifetime
    quint64 getLifetimePacketsSent() const { return _totalPacketsSent; }

    /// returns the total bytes sent by this object over its lifetime
    quint64 getLifetimeBytesSent() const { return _totalBytesSent; }

    /// returns the total packets queued by this object over its lifetime
    quint64 getLifetimePacketsQueued() const { return _totalPacketsQueued; }

    /// returns the total bytes queued by this object over its lifetime
    quint64 getLifetimeBytesQueued() const { return _totalBytesQueued; }
signals:
    void packetSent(quint64);
protected:
    int _packetsPerSecond;
    int _usecsPerProcessCallHint;
    quint64 _lastProcessCallTime;
    SimpleMovingAverage _averageProcessCallTime;

private:
    std::list<NodePacketPair> _packets;
    quint64 _lastSendTime;

    bool threadedProcess();
    bool nonThreadedProcess();

    quint64 _lastPPSCheck;
    int _packetsOverCheckInterval;

    quint64 _started;
    quint64 _totalPacketsSent;
    quint64 _totalBytesSent;

    quint64 _totalPacketsQueued;
    quint64 _totalBytesQueued;

    QWaitCondition _hasPackets;
    QMutex _waitingOnPacketsMutex;
};

#endif // hifi_PacketSender_h
