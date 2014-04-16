//
//  VoxelTreeCommands.h
//  libraries/voxels/src
//
//  Created by Clement on 4/4/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_VoxelTreeCommands_h
#define hifi_VoxelTreeCommands_h

#include <QRgb>
#include <QUndoCommand>

#include "VoxelDetail.h"
#include "VoxelEditPacketSender.h"

class VoxelTree;

class AddVoxelCommand : public QUndoCommand {
public:
    AddVoxelCommand(VoxelTree* tree, VoxelDetail& voxel, VoxelEditPacketSender* packetSender = NULL, QUndoCommand* parent = NULL);
    
    virtual void redo();
    virtual void undo();
    
private:
    VoxelTree* _tree;
    VoxelEditPacketSender* _packetSender;
    VoxelDetail _voxel;
};

class DeleteVoxelCommand : public QUndoCommand {
public:
    DeleteVoxelCommand(VoxelTree* tree, VoxelDetail& voxel, VoxelEditPacketSender* packetSender = NULL, QUndoCommand* parent = NULL);
    
    virtual void redo();
    virtual void undo();
    
private:
    VoxelTree* _tree;
    VoxelEditPacketSender* _packetSender;
    VoxelDetail _voxel;
};

#endif // hifi_VoxelTreeCommands_h
