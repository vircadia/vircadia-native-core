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
#include <QVector>

#include <glm/glm.hpp>

#include <NodeList.h>

#include <DatagramSequencer.h>
#include <MetavoxelData.h>
#include <MetavoxelMessages.h>

#include "renderer/ProgramObject.h"

class Model;

/// Renders a metavoxel tree.
class MetavoxelSystem : public QObject {
    Q_OBJECT

public:

    MetavoxelSystem();

    void init();
    
    void applyEdit(const MetavoxelEditMessage& edit);
    
    void simulate(float deltaTime);
    void render();
    
private slots:

    void maybeAttachClient(const SharedNodePointer& node);

private:
    
    class Point {
    public:
        glm::vec4 vertex;
        quint8 color[4];
        quint8 normal[3];
    };
    
    class SimulateVisitor : public SpannerVisitor {
    public:
        SimulateVisitor(QVector<Point>& points);
        void setDeltaTime(float deltaTime) { _deltaTime = deltaTime; }
        void setOrder(const glm::vec3& direction) { _order = encodeOrder(direction); }
        virtual bool visit(Spanner* spanner);
        virtual int visit(MetavoxelInfo& info);
    
    private:
        QVector<Point>& _points;
        float _deltaTime;
        int _order;
    };
    
    class RenderVisitor : public SpannerVisitor {
    public:
        RenderVisitor();
        virtual bool visit(Spanner* spanner);
    };
    
    static ProgramObject _program;
    static int _pointScaleLocation;
    
    QVector<Point> _points;
    SimulateVisitor _simulateVisitor;
    RenderVisitor _renderVisitor;
    QOpenGLBuffer _buffer;
};

/// A client session associated with a single server.
class MetavoxelClient : public NodeData {
    Q_OBJECT    
    
public:
    
    MetavoxelClient(const SharedNodePointer& node);
    virtual ~MetavoxelClient();

    void guide(MetavoxelVisitor& visitor);

    void applyEdit(const MetavoxelEditMessage& edit);

    void simulate(float deltaTime);

    virtual int parseData(const QByteArray& packet);

private slots:
    
    void sendData(const QByteArray& data);

    void readPacket(Bitstream& in);
    
    void clearSendRecordsBefore(int index);
    
    void clearReceiveRecordsBefore(int index);
    
private:
    
    void handleMessage(const QVariant& message, Bitstream& in);
    
    class SendRecord {
    public:
        int packetNumber;
        MetavoxelLOD lod;
    };
    
    class ReceiveRecord {
    public:
        int packetNumber;
        MetavoxelData data;
        MetavoxelLOD lod;
    };
    
    SharedNodePointer _node;
    
    DatagramSequencer _sequencer;
    
    MetavoxelData _data;
    
    QList<SendRecord> _sendRecords;
    QList<ReceiveRecord> _receiveRecords;
};

/// Renders spheres.
class SphereRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE SphereRenderer();
    
    virtual void render(float alpha);
};

/// Renders static models.
class StaticModelRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE StaticModelRenderer();
    
    virtual void init(Spanner* spanner);
    virtual void simulate(float deltaTime);
    virtual void render(float alpha);

private slots:

    void applyTranslation(const glm::vec3& translation);
    void applyRotation(const glm::vec3& rotation);
    void applyScale(float scale);
    void applyURL(const QUrl& url);

private:
    
    Model* _model;
};

#endif /* defined(__interface__MetavoxelSystem__) */
