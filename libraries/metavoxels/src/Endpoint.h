//
//  Endpoint.h
//  libraries/metavoxels/src
//
//  Created by Andrzej Kapolka on 6/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Endpoint_h
#define hifi_Endpoint_h

#include <NodeList.h>

#include "DatagramSequencer.h"
#include "MetavoxelData.h"

class PacketRecord;

/// Base class for communication endpoints: clients or servers.
class Endpoint : public NodeData {
    Q_OBJECT

public:
    
    Endpoint(const SharedNodePointer& node);
    virtual ~Endpoint();
    
    void update();
    
    virtual int parseData(const QByteArray& packet);

protected slots:

    virtual void sendDatagram(const QByteArray& data);
    virtual void readMessage(Bitstream& in);
    
    void clearSendRecordsBefore(int index);
    void clearReceiveRecordsBefore(int index);

protected:

    virtual void writeUpdateMessage(Bitstream& out);
    virtual void handleMessage(const QVariant& message, Bitstream& in); 
    
    virtual PacketRecord* maybeCreateSendRecord(bool baseline = false) const;
    virtual PacketRecord* maybeCreateReceiveRecord(bool baseline = false) const;
    
    PacketRecord* getLastAcknowledgedSendRecord() const { return _sendRecords.first(); }
    PacketRecord* getLastAcknowledgedReceiveRecord() const { return _receiveRecords.first(); }
    
    SharedNodePointer _node;
    DatagramSequencer _sequencer;
    
    QList<PacketRecord*> _sendRecords;
    QList<PacketRecord*> _receiveRecords;
};

/// Base class for packet records.
class PacketRecord {
public:

    PacketRecord(const MetavoxelLOD& lod = MetavoxelLOD());
    virtual ~PacketRecord();
    
    const MetavoxelLOD& getLOD() const { return _lod; }
    
private:
    
    MetavoxelLOD _lod;
};

#endif // hifi_Endpoint_h
