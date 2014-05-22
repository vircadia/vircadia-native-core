//
//  LocalVoxelsList.cpp
//  libraries/voxels/src
//
//  Created by Clément Brisset on 2/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LocalVoxelsList.h"

static void doNothing(VoxelTree* t) {
    // do nothing
}

LocalVoxelsList* LocalVoxelsList::_instance = NULL;

LocalVoxelsList* LocalVoxelsList::getInstance() {
    if (!_instance) {
        _instance = new LocalVoxelsList();
    }
    
    return _instance;
}

LocalVoxelsList::LocalVoxelsList() {
}

LocalVoxelsList::~LocalVoxelsList() {
    _instance = NULL;
}

StrongVoxelTreePointer LocalVoxelsList::getTree(QString treeName) {
    return _trees.value(treeName);
}

void LocalVoxelsList::addPersistantTree(QString treeName, VoxelTree* tree) {
    StrongVoxelTreePointer treePtr(tree, doNothing);
    _persistantTrees.push_back(treePtr);
    _trees.insert(treeName, treePtr);
}

void LocalVoxelsList::insert(QString treeName, StrongVoxelTreePointer& tree) {
    // If the key don't already exist or the value is null
    if (!_trees.contains(treeName) || !_trees.value(treeName)) {
        _trees.insert(treeName, tree);
        qDebug() << "LocalVoxelsList : added local tree (" << treeName << ")";
    } else {
        // if not we replace the tree created by the user with the existing one
        tree = _trees.value(treeName);
        qDebug() << "[WARNING] LocalVoxelsList : local tree already exist (" << treeName << ")";
    }
}

void LocalVoxelsList::remove(QString treeName) {
    // if the tree is not used anymore (no strong pointer)
    if (!_trees.value(treeName)) {
        // then remove it from the list
        _trees.remove(treeName);
    }
}
