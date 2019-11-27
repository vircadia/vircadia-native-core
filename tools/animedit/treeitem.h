//
//  TreeItem
//
//  Created by Anthony Thibault on 6/5/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TreeItem_h
#define hifi_TreeItem_h

#include <QList>
#include <QVariant>

class TreeItem
{
public:
    explicit TreeItem(const QList<QVariant>& data);
    ~TreeItem();

    void appendChild(TreeItem* child);
    int findChild(TreeItem* child);
    void removeChild(int index);
    void insertChild(int index, TreeItem* child);

    TreeItem* child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    bool setData(int column, const QVariant& value);
    int row() const;
    TreeItem* parentItem();

    TreeItem* cloneNode() const;
    TreeItem* cloneNodeAndChildren() const;

private:
    QList<TreeItem*> m_childItems;
    QList<QVariant> m_itemData;
    TreeItem* m_parentItem;
};

#endif
