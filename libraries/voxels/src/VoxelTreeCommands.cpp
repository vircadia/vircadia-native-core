//
//  VoxelTreeCommands.cpp
//  libraries/voxels/src
//
//  Created by Clement on 4/4/14.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "VoxelTree.h"

#include "VoxelTreeCommands.h"

AddVoxelCommand::AddVoxelCommand(VoxelTree* tree, VoxelDetail& voxel, VoxelEditPacketSender* packetSender, QUndoCommand* parent) :
    QUndoCommand("Add Voxel", parent),
    _tree(tree),
    _packetSender(packetSender),
    _voxel(voxel),
    _oldTree(NULL)
{
    VoxelTreeElement* element = _tree->getVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
    if (element) {
        _oldTree = new VoxelTree();
        _tree->copySubTreeIntoNewTree(element, _oldTree, true);
    }
}

AddVoxelCommand::~AddVoxelCommand() {
    delete _oldTree;
}

void AddVoxelCommand::redo() {
    if (_tree) {
        _tree->createVoxel(_voxel.x, _voxel.y, _voxel.z, _voxel.s, _voxel.red, _voxel.green, _voxel.blue);
    }
    if (_packetSender) {
        _packetSender->queueVoxelEditMessages(PacketTypeVoxelSet, 1, &_voxel);
    }
}

void AddVoxelCommand::undo() {
    if (_oldTree) {
        OctreeElement* element = _tree->getOrCreateChildElementAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
        qDebug() << "undo(): " << _voxel.x << " " << _voxel.y << " " << _voxel.z << " " << _voxel.s;
        _tree->copyFromTreeIntoSubTree(_oldTree, element);
        qDebug() << "done";
    } else {
        if (_tree) {
            _tree->deleteVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
        }
        if (_packetSender) {
            _packetSender->queueVoxelEditMessages(PacketTypeVoxelErase, 1, &_voxel);
        }
    }
}

DeleteVoxelCommand::DeleteVoxelCommand(VoxelTree* tree, VoxelDetail& voxel, VoxelEditPacketSender* packetSender, QUndoCommand* parent) :
    QUndoCommand("Delete Voxel", parent),
    _tree(tree),
    _packetSender(packetSender),
    _voxel(voxel),
    _oldTree(NULL)
{
}

DeleteVoxelCommand::~DeleteVoxelCommand() {
    delete _oldTree;
}

void DeleteVoxelCommand::redo() {
    if (_oldTree) {
        
    } else {
        if (_tree) {
            _tree->deleteVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
        }
        if (_packetSender) {
            _packetSender->queueVoxelEditMessages(PacketTypeVoxelErase, 1, &_voxel);
        }
    }
}

void DeleteVoxelCommand::undo() {
    if (_oldTree) {
        
    } else {
        if (_tree) {
            _tree->createVoxel(_voxel.x, _voxel.y, _voxel.z, _voxel.s, _voxel.red, _voxel.green, _voxel.blue);
        }
        if (_packetSender) {
            _packetSender->queueVoxelEditMessages(PacketTypeVoxelSet, 1, &_voxel);
        }
    }
}