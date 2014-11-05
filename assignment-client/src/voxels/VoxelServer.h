//
//  VoxelServer.h
//  assignment-client/src/voxels
//
//  Created by Brad Hefta-Gaub on 8/21/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelServer_h
#define hifi_VoxelServer_h

#include <QStringList>
#include <QDateTime>
#include <QtCore/QCoreApplication>

#include <ThreadedAssignment.h>
#include <EnvironmentData.h>

#include "../octree/OctreeServer.h"

#include "VoxelServerConsts.h"

/// Handles assignments of type VoxelServer - sending voxels to various clients.
class VoxelServer : public OctreeServer {
public:
    VoxelServer(const QByteArray& packet);
    ~VoxelServer();

    bool wantSendEnvironments() const { return _sendEnvironments; }
    bool getSendMinimalEnvironment() const { return _sendMinimalEnvironment; }
    EnvironmentData* getEnvironmentData(int i) { return &_environmentData[i]; }
    int getEnvironmentDataCount() const { return sizeof(_environmentData)/sizeof(EnvironmentData); }

    // Subclasses must implement these methods
    virtual OctreeQueryNode* createOctreeQueryNode();
    virtual Octree* createTree();
    virtual char getMyNodeType() const { return NodeType::VoxelServer; }
    virtual PacketType getMyQueryMessageType() const { return PacketTypeVoxelQuery; }
    virtual const char* getMyServerName() const { return VOXEL_SERVER_NAME; }
    virtual const char* getMyLoggingServerTargetName() const { return VOXEL_SERVER_LOGGING_TARGET_NAME; }
    virtual const char* getMyDefaultPersistFilename() const { return LOCAL_VOXELS_PERSIST_FILE; }
    virtual PacketType getMyEditNackType() const { return PacketTypeVoxelEditNack; }
    virtual QString getMyDomainSettingsKey() const { return QString("voxel_server_settings"); }

    // subclass may implement these method
    virtual bool hasSpecialPacketToSend(const SharedNodePointer& node);
    virtual int sendSpecialPacket(const SharedNodePointer& node, OctreeQueryNode* queryNode, int& packetsSent);

protected:
    virtual void readAdditionalConfiguration(const QJsonObject& settingsSectionObject);

private:
    bool _sendEnvironments;
    bool _sendMinimalEnvironment;
    EnvironmentData _environmentData[3];
    unsigned char _tempOutputBuffer[MAX_PACKET_SIZE];
};

#endif // hifi_VoxelServer_h
