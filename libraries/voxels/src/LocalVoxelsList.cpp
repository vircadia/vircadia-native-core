//
//  LocalVoxelsList.cpp
//  hifi
//
//  Created by Clément Brisset on 2/24/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
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
    qDebug() << "[DEBUG] LocalVoxelsList : added persistant tree (" << treeName << ")" << endl;
}

void LocalVoxelsList::insert(QString treeName, StrongVoxelTreePointer& tree) {
    // If the key don't already exist or the value is null
    if (!_trees.contains(treeName) || !_trees.value(treeName)) {
        _trees.insert(treeName, tree);
        qDebug() << "[DEBUG] LocalVoxelsList : added local tree (" << treeName << ")" << endl;
    } else {
        // if not we replace the tree created by the user with the existing one
        tree = _trees.value(treeName);
        qDebug() << "[DEBUG] LocalVoxelsList : local tree already exist (" << treeName << ")"<< endl;
    }
}

void LocalVoxelsList::remove(QString treeName) {
    // if the tree is not used anymore (no strong pointer)
    if (!_trees.value(treeName, StrongVoxelTreePointer(NULL))) {
        // then remove it from the list
        qDebug() << "[DEBUG] LocalVoxelsList : removed unused tree (" << treeName << ")" << endl;
        _trees.remove(treeName);
    } else {
        qDebug() << "[DEBUG] LocalVoxelsList : tree still in use (" << treeName << ")" << endl;
    }
}