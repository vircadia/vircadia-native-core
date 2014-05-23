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



struct SendVoxelsOperationArgs {
    const unsigned char*  newBaseOctCode;
    VoxelEditPacketSender* packetSender;
};

bool sendVoxelsOperation(OctreeElement* element, void* extraData) {
    VoxelTreeElement* voxel = static_cast<VoxelTreeElement*>(element);
    SendVoxelsOperationArgs* args = static_cast<SendVoxelsOperationArgs*>(extraData);
    if (voxel->isColored()) {
        const unsigned char* nodeOctalCode = voxel->getOctalCode();
        unsigned char* codeColorBuffer = NULL;
        int codeLength  = 0;
        int bytesInCode = 0;
        int codeAndColorLength;
        
        // If the newBase is NULL, then don't rebase
        if (args->newBaseOctCode) {
            codeColorBuffer = rebaseOctalCode(nodeOctalCode, args->newBaseOctCode, true);
            codeLength  = numberOfThreeBitSectionsInCode(codeColorBuffer);
            bytesInCode = bytesRequiredForCodeLength(codeLength);
            codeAndColorLength = bytesInCode + SIZE_OF_COLOR_DATA;
        } else {
            codeLength  = numberOfThreeBitSectionsInCode(nodeOctalCode);
            bytesInCode = bytesRequiredForCodeLength(codeLength);
            codeAndColorLength = bytesInCode + SIZE_OF_COLOR_DATA;
            codeColorBuffer = new unsigned char[codeAndColorLength];
            memcpy(codeColorBuffer, nodeOctalCode, bytesInCode);
        }
        
        // copy the colors over
        codeColorBuffer[bytesInCode + RED_INDEX] = voxel->getColor()[RED_INDEX];
        codeColorBuffer[bytesInCode + GREEN_INDEX] = voxel->getColor()[GREEN_INDEX];
        codeColorBuffer[bytesInCode + BLUE_INDEX] = voxel->getColor()[BLUE_INDEX];
        args->packetSender->queueVoxelEditMessage(PacketTypeVoxelSetDestructive,
                                                  codeColorBuffer, codeAndColorLength);
        
        delete[] codeColorBuffer;
    }
    return true; // keep going
}


AddVoxelCommand::AddVoxelCommand(VoxelTree* tree, VoxelDetail& voxel, VoxelEditPacketSender* packetSender, QUndoCommand* parent) :
    QUndoCommand("Add Voxel", parent),
    _tree(tree),
    _packetSender(packetSender),
    _voxel(voxel)
{
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
    if (_tree) {
        _tree->deleteVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
    }
    if (_packetSender) {
        _packetSender->queueVoxelEditMessages(PacketTypeVoxelErase, 1, &_voxel);
    }
}

DeleteVoxelCommand::DeleteVoxelCommand(VoxelTree* tree, VoxelDetail& voxel, VoxelEditPacketSender* packetSender, QUndoCommand* parent) :
    QUndoCommand("Delete Voxel", parent),
    _tree(tree),
    _packetSender(packetSender),
    _voxel(voxel),
    _oldTree(NULL)
{
    _tree->lockForRead();
    VoxelTreeElement* element = _tree->getEnclosingVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
    if (element->getScale() == _voxel.s) {
        if (!element->hasContent() && !element->isLeaf()) {
            _oldTree = new VoxelTree();
            _tree->copySubTreeIntoNewTree(element, _oldTree, false);
        } else {
            _voxel.red = element->getColor()[0];
            _voxel.green = element->getColor()[1];
            _voxel.blue = element->getColor()[2];
        }
    } else if (element->hasContent() && element->isLeaf()) {
        _voxel.red = element->getColor()[0];
        _voxel.green = element->getColor()[1];
        _voxel.blue = element->getColor()[2];
    } else {
        _voxel.s = 0.0f;
    }
    _tree->unlock();
}

DeleteVoxelCommand::~DeleteVoxelCommand() {
    delete _oldTree;
}

void DeleteVoxelCommand::redo() {
    if (_voxel.s == 0.0f) {
        return;
    }
    
    if (_tree) {
        _tree->deleteVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
    }
    if (_packetSender) {
        _packetSender->queueVoxelEditMessages(PacketTypeVoxelErase, 1, &_voxel);
    }
}

void DeleteVoxelCommand::undo() {
    if (_voxel.s == 0.0f) {
        return;
    }
    
    if (_oldTree) {
        VoxelTreeElement* element = _oldTree->getVoxelAt(_voxel.x, _voxel.y, _voxel.z, _voxel.s);
        if (element) {
            if (_tree) {
                _tree->lockForWrite();
                _oldTree->copySubTreeIntoNewTree(element, _tree, false);
                _tree->unlock();
            }
            if (_packetSender) {
                SendVoxelsOperationArgs args;
                args.newBaseOctCode = NULL;
                args.packetSender = _packetSender;
                _oldTree->recurseTreeWithOperation(sendVoxelsOperation, &args);
                _packetSender->releaseQueuedMessages();
            }
        }
    } else {
        if (_tree) {
            _tree->createVoxel(_voxel.x, _voxel.y, _voxel.z, _voxel.s, _voxel.red, _voxel.green, _voxel.blue);
        }
        if (_packetSender) {
            _packetSender->queueVoxelEditMessages(PacketTypeVoxelSet, 1, &_voxel);
        }
    }
}
