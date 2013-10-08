//
//  PacketSender.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded packet sender.
//

#ifndef __shared__PacketSender__
#define __shared__PacketSender__

#include "GenericThread.h"
#include "NetworkPacket.h"

/// Notification Hook for packets being sent by a PacketSender
class PacketSenderNotify {
public:
    virtual void packetSentNotification(ssize_t length) = 0;
};


/// Generalized threaded processor for queueing and sending of outbound packets. 
class PacketSender : public virtual GenericThread {
public:
    static const int DEFAULT_PACKETS_PER_SECOND;
    static const int MINIMUM_PACKETS_PER_SECOND;

    PacketSender(PacketSenderNotify* notify = NULL, int packetsPerSecond = DEFAULT_PACKETS_PER_SECOND);

    /// Add packet to outbound queue.
    /// \param sockaddr& address the destination address
    /// \param packetData pointer to data
    /// \param ssize_t packetLength size of data
    /// \thread any thread, typically the application thread
    void queuePacketForSending(sockaddr& address, unsigned char*  packetData, ssize_t packetLength);
    
    void setPacketsPerSecond(int packetsPerSecond) { _packetsPerSecond = std::max(MINIMUM_PACKETS_PER_SECOND, packetsPerSecond); }
    int getPacketsPerSecond() const { return _packetsPerSecond; }

    void setPacketSenderNotify(PacketSenderNotify* notify) { _notify = notify; }
    PacketSenderNotify* getPacketSenderNotify() const { return _notify; }

    virtual bool process();

    /// are there packets waiting in the send queue to be sent
    bool hasPacketsToSend() const { return _packets.size() > 0; }

    /// how many packets are there in the send queue waiting to be sent
    int packetsToSendCount() const { return _packets.size(); }

    /// If you're running in non-threaded mode, call this to give us a hint as to how frequently you will call process.
    /// This has no effect in threaded mode. This is only considered a hint in non-threaded mode.
    /// \param int usecsPerProcessCall expected number of usecs between calls to process in non-threaded mode.
    void setProcessCallIntervalHint(int usecsPerProcessCall) { _usecsPerProcessCallHint = usecsPerProcessCall; }

protected:
    int _packetsPerSecond;
    int _usecsPerProcessCallHint;
    uint64_t _lastProcessCallTime;
    SimpleMovingAverage _averageProcessCallTime;
    
private:
    std::vector<NetworkPacket> _packets;
    uint64_t _lastSendTime;
    PacketSenderNotify* _notify;
};

#endif // __shared__PacketSender__
