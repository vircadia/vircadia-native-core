//
//  LocalVoxelsList.h
//  hifi
//
//  Created by Clément Brisset on 2/24/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__LocalVoxelsList__
#define __hifi__LocalVoxelsList__

#include <QHash>
#include <QString>
#include <QSharedPointer>
#include <QWeakPointer>

#include "VoxelTree.h"

typedef QSharedPointer<VoxelTree> StrongVoxelTreePointer;
typedef QWeakPointer<VoxelTree> WeakVoxelTreePointer;

/// Handles the the storage and cleanup of local named trees used by JS
class LocalVoxelsList {
public:
    static LocalVoxelsList* getInstance();
    ~LocalVoxelsList();
    
    /// Lookup up a tree in the QHash and return a strong pointer to it.
    /// \param treeName name of the tree to look up
    StrongVoxelTreePointer getTree(QString treeName);
    
    /// Add a that will stay in the list until destruction of the instance and won't be destroyed then either.
    /// \param treeName name to give to the tree in the list
    /// \param tree standard pointer to the tree
    void addPersistantTree(QString treeName, VoxelTree* tree);
    
    /// insert a local tree in the list
    /// \param treeName name to give to the tree in the list
    /// \param tree strong pointer to the tree
    void insert(QString treeName, StrongVoxelTreePointer& tree);
    
    /// remove a tree from the list if it's not being used anymore
    /// \param treeName name of the tree to remove
    void remove(QString treeName);
    
private:
    static LocalVoxelsList* _instance;
    LocalVoxelsList();
    
    QHash<QString, WeakVoxelTreePointer> _trees;
    
    QList<StrongVoxelTreePointer> _persistantTree;
};

#endif /* defined(__hifi__LocalVoxelsList__) */
