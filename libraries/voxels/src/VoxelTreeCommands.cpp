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
        if (element->hasContent() && element->isLeaf()) {
            // Do nothing, everything is in order
        } else {
            _oldTree = new VoxelTree();
            _tree->copySubTreeIntoNewTree(element, _oldTree, true);
        }
    } else {
        glm::vec3 point(_voxel.x + _voxel.s / 2.0f,
                        _voxel.y + _voxel.s / 2.0f,
                        _voxel.z + _voxel.s / 2.0f);
        OctreeElement* element = _tree->getElementEnclosingPoint(point, Octree::Lock);
        if (element) {
            VoxelTreeElement* node = static_cast<VoxelTreeElement*>(element);
            _voxel.x = node->getCorner().x;
            _voxel.y = node->getCorner().y;
            _voxel.z = node->getCorner().z;
            _voxel.s = node->getScale();
            _voxel.red = node->getColor()[0];
            _voxel.green = node->getColor()[1];
            _voxel.blue = node->getColor()[2];
        }
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
        qDebug() << _voxel.x << " " << _voxel.y << " " << _voxel.z << " " << _voxel.s;
        _tree->copyFromTreeIntoSubTree(_oldTree, element);
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