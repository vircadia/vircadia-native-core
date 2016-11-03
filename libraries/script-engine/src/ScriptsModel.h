//
//  ScriptsModel.h
//  interface/src
//
//  Created by Ryan Huffman on 05/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptsModel_h
#define hifi_ScriptsModel_h

#include <QAbstractItemModel>
#include <QDir>
#include <QNetworkReply>
#include <QFileSystemWatcher>

class TreeNodeFolder;

enum ScriptOrigin {
    SCRIPT_ORIGIN_LOCAL,
    SCRIPT_ORIGIN_DEFAULT
};

enum TreeNodeType {
    TREE_NODE_TYPE_SCRIPT,
    TREE_NODE_TYPE_FOLDER
};

class TreeNodeBase {
public:
    TreeNodeFolder* getParent() const { return _parent; }
    void setParent(TreeNodeFolder* parent) { _parent = parent; }
    TreeNodeType getType() { return _type; }
    const QString& getName() { return _name; };

private:
    TreeNodeFolder* _parent;
    TreeNodeType _type;

protected:
    QString _name;
    TreeNodeBase(TreeNodeFolder* parent, const QString& name, TreeNodeType type);
};

class TreeNodeScript : public TreeNodeBase {
public:
    TreeNodeScript(const QString& localPath, const QString& fullPath, ScriptOrigin origin);
    const QString& getLocalPath() { return _localPath; }
    const QString& getFullPath() { return _fullPath; };
    ScriptOrigin getOrigin() { return _origin; };

private:
    QString _localPath;
    QString _fullPath;
    ScriptOrigin _origin;
};

class TreeNodeFolder : public TreeNodeBase {
public:
    TreeNodeFolder(const QString& foldername, TreeNodeFolder* parent);
};

class ScriptsModel : public QAbstractItemModel {
    Q_OBJECT
public:
    ScriptsModel(QObject* parent = NULL);
    ~ScriptsModel();
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    TreeNodeBase* getTreeNodeFromIndex(const QModelIndex& index) const;
    QList<TreeNodeBase*> getFolderNodes(TreeNodeFolder* parent) const;

    enum Role {
        ScriptPath = Qt::UserRole,
    };

protected slots:
    void updateScriptsLocation(const QString& newPath);
    void downloadFinished();
    void reloadLocalFiles();
    void reloadDefaultFiles();

protected:
    void requestDefaultFiles(QString marker = QString());
    bool parseXML(QByteArray xmlFile);
    void rebuildTree();

private:
    friend class ScriptEngines;
    bool _loadingScripts;
    QDir _localDirectory;
    QFileSystemWatcher _fsWatcher;
    QList<TreeNodeBase*> _treeNodes;
};

#endif // hifi_ScriptsModel_h
