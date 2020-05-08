//
//  TreeModel
//
//  Created by Anthony Thibault on 6/5/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TreeModel_h
#define hifi_TreeModel_h

#include <QAbstractItemModel>
#include <QJsonObject>
#include <QModelIndex>
#include <QObject>
#include <QString>
#include <QVariant>

class TreeItem;

class TreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    enum TreeModelRoles
    {
        TreeModelRoleName = Qt::UserRole + 1,
        TreeModelRoleType,
        TreeModelRoleData
    };

    explicit TreeModel(QObject* parent = nullptr);
    ~TreeModel() override;

    // QAbstractItemModel interface
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    // read methods
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = TreeModelRoleName) const override;
    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    // write methods
    bool setData(const QModelIndex& index, const QVariant& value, int role = TreeModelRoleName) override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role = TreeModelRoleName) override;
    bool insertColumns(int position, int columns, const QModelIndex& parent = QModelIndex()) override;
    bool removeColumns(int position, int columns, const QModelIndex& parent = QModelIndex()) override;
    bool insertRows(int position, int rows, const QModelIndex& parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex& parent = QModelIndex()) override;

    // invokable from qml
    Q_INVOKABLE void loadFromFile(const QString& filename);
    Q_INVOKABLE void saveToFile(const QString& filename);
    Q_INVOKABLE void newNode(const QModelIndex& parent);
    Q_INVOKABLE void deleteNode(const QModelIndex& index);
    Q_INVOKABLE void insertNodeAbove(const QModelIndex& index);
    Q_INVOKABLE QVariantList getChildrenModelIndices(const QModelIndex& index);
    Q_INVOKABLE void copyNode(const QModelIndex& index);
    Q_INVOKABLE void copyNodeAndChildren(const QModelIndex& index);
    Q_INVOKABLE void pasteOver(const QModelIndex& index);
    Q_INVOKABLE void pasteAsChild(const QModelIndex& index);

private:
    TreeItem* loadNode(const QJsonObject& jsonObj);
    TreeItem* getItem(const QModelIndex& index) const;
    QJsonObject jsonFromItem(TreeItem* treeItem);

    TreeItem* _rootItem;
    QHash<int, QByteArray> _roleNameMapping;
    TreeItem* _clipboard;
};

#endif
