//
//  ScriptsModelFilter.cpp
//  interface/src
//
//  Created by Thijs Wenker on 01/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptsModelFilter.h"

ScriptsModelFilter::ScriptsModelFilter(QObject *parent) : 
    QSortFilterProxyModel(parent) {
}

bool ScriptsModelFilter::lessThan(const QModelIndex& left, const QModelIndex& right) const {
    ScriptsModel* scriptsModel = static_cast<ScriptsModel*>(sourceModel());
    TreeNodeBase* leftNode = scriptsModel->getTreeNodeFromIndex(left);
    TreeNodeBase* rightNode = scriptsModel->getTreeNodeFromIndex(right);
    if (leftNode->getType() != rightNode->getType()) {
        return leftNode->getType() == TREE_NODE_TYPE_FOLDER;
    }
    return leftNode->getName() < rightNode->getName();
}

bool ScriptsModelFilter::filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const {
    if (!filterRegExp().isEmpty()) {
        ScriptsModel* scriptsModel = static_cast<ScriptsModel*>(sourceModel());
        TreeNodeBase* node = scriptsModel->getFolderNodes(
            static_cast<TreeNodeFolder*>(scriptsModel->getTreeNodeFromIndex(sourceParent))).at(sourceRow);
        QModelIndex sourceIndex = sourceModel()->index(sourceRow, this->filterKeyColumn(), sourceParent);
        if (node->getType() == TREE_NODE_TYPE_FOLDER) {
            int rows = scriptsModel->rowCount(sourceIndex);
            for (int i = 0; i < rows; i++) {
                if (filterAcceptsRow(i, sourceIndex)) {
                    return true;
                }
            }
        }
    }
    return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
