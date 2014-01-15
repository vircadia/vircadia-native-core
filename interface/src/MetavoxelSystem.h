//
//  MetavoxelSystem.h
//  interface
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelSystem__
#define __interface__MetavoxelSystem__

#include <QList>
#include <QOpenGLBuffer>
#include <QScriptEngine>
#include <QVector>

#include <glm/glm.hpp>

#include <NodeList.h>

#include <DatagramSequencer.h>
#include <MetavoxelData.h>

#include "renderer/ProgramObject.h"

class MetavoxelClient;

/// Renders a metavoxel tree.
class MetavoxelSystem : public QObject {
    Q_OBJECT

public:
    MetavoxelSystem();

    void init();
    
    void processData(const QByteArray& data, const HifiSockAddr& sender);
    
    void simulate(float deltaTime);
    void render();
    
public slots:
    void nodeAdded(SharedNodePointer node);
    void nodeKilled(SharedNodePointer node);
private:

    Q_INVOKABLE void addClient(const QUuid& uuid, const HifiSockAddr& address);
    Q_INVOKABLE void removeClient(const QUuid& uuid);
    Q_INVOKABLE void receivedData(const QByteArray& data, const HifiSockAddr& sender);

    class Point {
    public:
        glm::vec4 vertex;
        quint8 color[4];
        quint8 normal[3];
    };
    
    class PointVisitor : public MetavoxelVisitor {
    public:
        PointVisitor(QVector<Point>& points);
        virtual bool visit(const MetavoxelInfo& info);
    
    private:
        QVector<Point>& _points;
    };
    
    static ProgramObject _program;
    static int _pointScaleLocation;
    
    QScriptEngine _scriptEngine;
    MetavoxelData _data;
    QVector<Point> _points;
    PointVisitor _pointVisitor;
    QOpenGLBuffer _buffer;
    
    QHash<QUuid, MetavoxelClient*> _clients;
    QHash<QUuid, MetavoxelClient*> _clientsBySessionID;
};

/// A client session associated with a single server.
class MetavoxelClient : public QObject {
    Q_OBJECT    
    
public:
    
    MetavoxelClient(const HifiSockAddr& address);

    const QUuid& getSessionID() const { return _sessionID; }

    void simulate(float deltaTime);

    void receivedData(const QByteArray& data, const HifiSockAddr& sender);

private slots:
    
    void sendData(const QByteArray& data);

    void readPacket(Bitstream& in);
    
    void clearReceiveRecordsBefore(int index);
    
private:
    
    void handleMessage(const QVariant& message, Bitstream& in);
    
    class ReceiveRecord {
    public:
        int packetNumber;
        MetavoxelDataPointer data;
    };
    
    HifiSockAddr _address;
    QUuid _sessionID;
    
    DatagramSequencer _sequencer;
    
    MetavoxelDataPointer _data;
    
    QList<ReceiveRecord> _receiveRecords;
};

#endif /* defined(__interface__MetavoxelSystem__) */
