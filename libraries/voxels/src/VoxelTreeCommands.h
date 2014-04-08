//
//  VoxelTreeCommands.h
//  hifi
//
//  Created by Clement on 4/4/14.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__VoxelTreeCommands__
#define __hifi__VoxelTreeCommands__

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

#endif /* defined(__hifi__VoxelTreeCommands__) */
