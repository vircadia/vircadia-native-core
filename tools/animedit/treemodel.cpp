//
//  TreeModel
//
//  Created by Anthony Thibault on 6/5/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "treeitem.h"

#include <QDebug>
#include <QFile>
#include <QtGlobal>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QJSValue>

#include "treemodel.h"

static TreeItem* newBlankTreeItem() {
    QList<QVariant> columnData;
    columnData << "newNode";
    columnData << "clip";
    columnData << QJsonObject();  // blank
    return new TreeItem(columnData);
}

TreeModel::TreeModel(QObject* parent) : QAbstractItemModel(parent) {
    _roleNameMapping[TreeModelRoleName] = "name";
    _roleNameMapping[TreeModelRoleType] = "type";
    _roleNameMapping[TreeModelRoleData] = "data";

    QList<QVariant> rootData;
    rootData << "Name" << "Type" << "Data";
    _rootItem = new TreeItem(rootData);
    _clipboard = nullptr;
}

TreeModel::~TreeModel() {
    delete _rootItem;
}

QHash<int, QByteArray> TreeModel::roleNames() const {
    return _roleNameMapping;
}

Qt::ItemFlags TreeModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index);
}

QVariant TreeModel::data(const QModelIndex& index, int role) const {
    TreeItem* item = getItem(index);
    return item->data(role - Qt::UserRole - 1);
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return _rootItem->data(section);
    }

    return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex& parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    TreeItem *parentItem;

    if (!parent.isValid()) {
        parentItem = _rootItem;
    } else {
        parentItem = static_cast<TreeItem*>(parent.internalPointer());
    }

    TreeItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    } else {
        return QModelIndex();
    }
}

QModelIndex TreeModel::parent(const QModelIndex& index) const {
    if (!index.isValid()) {
        return QModelIndex();
    }

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == _rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex& parent) const {
    TreeItem* parentItem = getItem(parent);
    return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex& parent) const {
    return _rootItem->columnCount();
}

bool TreeModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    TreeItem* item = getItem(index);

    bool returnValue = item->setData(role - Qt::UserRole - 1, value);

    emit dataChanged(index, index);

    return returnValue;
}

bool TreeModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role) {
    return false;
}

bool TreeModel::insertColumns(int position, int columns, const QModelIndex& parent) {
    return false;
}

bool TreeModel::removeColumns(int position, int columns, const QModelIndex& parent) {
    return false;
}

bool TreeModel::insertRows(int position, int rows, const QModelIndex& parent) {
    return false;
}

bool TreeModel::removeRows(int position, int rows, const QModelIndex& parent) {
    return false;
}

void TreeModel::loadFromFile(const QString& filename) {

    beginResetModel();

    QFile file(filename);
    if (!file.exists()) {
        qCritical() << "TreeModel::loadFromFile, failed to open file" << filename;
    } else if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "TreeModel::loadFromFile, failed to open file" << filename;
    } else {
        qDebug() << "TreeModel::loadFromFile, success opening file" << filename;
        QByteArray contents = file.readAll();
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(contents, &error);
        if (error.error != QJsonParseError::NoError) {
            qCritical() << "TreeModel::loadFromFile, failed to parse json, error" << error.errorString();
        } else {
            QJsonObject obj = doc.object();

            // version
            QJsonValue versionVal = obj.value("version");
            if (!versionVal.isString()) {
                qCritical() << "TreeModel::loadFromFile, bad string \"version\"";
                return;
            }
            QString version = versionVal.toString();

            // check version
            if (version != "1.0" && version != "1.1") {
                qCritical() << "TreeModel::loadFromFile, bad version number" << version << "expected \"1.0\" or \"1.1\"";
                return;
            }

            // root
            QJsonValue rootVal = obj.value("root");
            if (!rootVal.isObject()) {
                qCritical() << "TreeModel::loadFromFile, bad object \"root\"";
                return;
            }

            QList<QVariant> columnData;
            columnData << QString("root");
            columnData << QString("root");
            columnData << QString("root");

            // create root item
            _rootItem = new TreeItem(columnData);
            _rootItem->appendChild(loadNode(rootVal.toObject()));
        }
    }

    endResetModel();
}

void TreeModel::saveToFile(const QString& filename) {

    QJsonObject obj;
    obj.insert("version", "1.1");

    const int FIRST_CHILD = 0;
    obj.insert("root", jsonFromItem(_rootItem->child(FIRST_CHILD)));

    QJsonDocument doc(obj);
    QByteArray byteArray = doc.toJson(QJsonDocument::Indented);

    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "TreeModel::safeToFile, failed to open file" << filename;
    } else {
        file.write(byteArray);
    }
}

void TreeModel::newNode(const QModelIndex& parent) {
    TreeItem* parentItem = _rootItem;
    if (parent.isValid()) {
        parentItem = static_cast<TreeItem*>(parent.internalPointer());
    }

    beginInsertRows(parent, parentItem->childCount(), parentItem->childCount());
    TreeItem* childItem = newBlankTreeItem();
    parentItem->appendChild(childItem);

    endInsertRows();
}

void TreeModel::deleteNode(const QModelIndex& index) {
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    TreeItem* parentItem = item->parentItem();
    int childNum = parentItem->findChild(item);

    if (childNum >= 0) {
        beginRemoveRows(createIndex(0, 0, reinterpret_cast<quintptr>(parentItem)), childNum, childNum);
        parentItem->removeChild(childNum);
        endRemoveRows();
    }
}

void TreeModel::insertNodeAbove(const QModelIndex& index) {
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    TreeItem* parentItem = item->parentItem();
    int childNum = parentItem->findChild(item);

    if (childNum >= 0) {
        TreeItem* newItem = newBlankTreeItem();
        QModelIndex parentIndex = createIndex(0, 0, reinterpret_cast<quintptr>(parentItem));

        // remove item
        beginRemoveRows(parentIndex, childNum, childNum);
        parentItem->removeChild(childNum);
        endRemoveRows();

        // append item to newItem
        newItem->appendChild(item);

        // then insert newItem
        beginInsertRows(parentIndex, childNum, childNum);
        parentItem->insertChild(childNum, newItem);
        endInsertRows();
    }
}

QVariantList TreeModel::getChildrenModelIndices(const QModelIndex& index) {
    QVariantList indices;

    TreeItem* parent = static_cast<TreeItem*>(index.internalPointer());
    for (int i = 0; i < parent->childCount(); ++i) {
        TreeItem* child = parent->child(i);
        indices.push_back(createIndex(i, 0, reinterpret_cast<quintptr>(child)));
    }
    return indices;
}

void TreeModel::copyNode(const QModelIndex& index) {
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    // TODO: delete previous clipboard
    _clipboard = item->cloneNode();
}

void TreeModel::copyNodeAndChildren(const QModelIndex& index) {
    TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
    // TODO: delete previous clipboard
    _clipboard = item->cloneNodeAndChildren();
}

void TreeModel::pasteOver(const QModelIndex& index) {
    if (_clipboard) {
        TreeItem* item = static_cast<TreeItem*>(index.internalPointer());
        TreeItem* parentItem = item->parentItem();
        int childNum = parentItem->findChild(item);

        if (childNum >= 0) {
            QModelIndex parentIndex = createIndex(0, 0, reinterpret_cast<quintptr>(parentItem));

            // remove item
            beginRemoveRows(parentIndex, childNum, childNum);
            parentItem->removeChild(childNum);
            endRemoveRows();

            // then insert clone of _clipboard
            beginInsertRows(parentIndex, childNum, childNum);
            parentItem->insertChild(childNum, _clipboard->cloneNodeAndChildren());
            endInsertRows();
        }
    }
}

void TreeModel::pasteAsChild(const QModelIndex& index) {
    if (_clipboard) {
        TreeItem* parentItem = _rootItem;
        if (index.isValid()) {
            parentItem = static_cast<TreeItem*>(index.internalPointer());
        }

        beginInsertRows(index, parentItem->childCount(), parentItem->childCount());
        parentItem->appendChild(_clipboard->cloneNodeAndChildren());

        endInsertRows();
    }
}

TreeItem* TreeModel::loadNode(const QJsonObject& jsonObj) {

    // id
    auto idVal = jsonObj.value("id");
    if (!idVal.isString()) {
        qCritical() << "loadNode, bad string \"id\"";
        return nullptr;
    }
    QString id = idVal.toString();

    // type
    auto typeVal = jsonObj.value("type");
    if (!typeVal.isString()) {
        qCritical() << "loadNode, bad object \"type\", id =" << id;
        return nullptr;
    }
    QString typeStr = typeVal.toString();

    // data
    auto dataValue = jsonObj.value("data");
    if (!dataValue.isObject()) {
        qCritical() << "AnimNodeLoader, bad string \"data\", id =" << id;
        return nullptr;
    }

    QList<QVariant> columnData;
    columnData << id;
    columnData << typeStr;
    columnData << dataValue.toVariant();

    // create node
    TreeItem* node = new TreeItem(columnData);

    // children
    auto childrenValue = jsonObj.value("children");
    if (!childrenValue.isArray()) {
        qCritical() << "AnimNodeLoader, bad array \"children\", id =" << id;
        return nullptr;
    }
    auto childrenArray = childrenValue.toArray();
    for (const auto& childValue : childrenArray) {
        if (!childValue.isObject()) {
            qCritical() << "AnimNodeLoader, bad object in \"children\", id =" << id;
            return nullptr;
        }
        TreeItem* child = loadNode(childValue.toObject());
        if (child) {
            node->appendChild(child);
        } else {
            return nullptr;
        }
    }

    return node;
}

TreeItem* TreeModel::getItem(const QModelIndex& index) const {
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
        if (item) {
            return item;
        }
    }
    return _rootItem;
}

QJsonObject TreeModel::jsonFromItem(TreeItem* treeItem) {
    QJsonObject obj;

    const int ID_COLUMN = 0;
    obj.insert("id", treeItem->data(ID_COLUMN).toJsonValue());

    const int TYPE_COLUMN = 1;
    obj.insert("type", treeItem->data(TYPE_COLUMN).toJsonValue());

    const int DATA_COLUMN = 2;
    QVariant data = treeItem->data(DATA_COLUMN);
    if (data.canConvert<QJSValue>()) {
        obj.insert("data", data.value<QJSValue>().toVariant().toJsonValue());
    } else {
        obj.insert("data", data.toJsonValue());
    }

    QJsonArray children;
    for (int i = 0; i < treeItem->childCount(); i++) {
        children.push_back(jsonFromItem(treeItem->child(i)));
    }
    obj.insert("children", children);

    return obj;
}
