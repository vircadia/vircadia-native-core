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
    SCRIPT_ORIGIN_REMOTE
};

enum TreeNodeType {
    TREE_NODE_TYPE_SCRIPT,
    TREE_NODE_TYPE_FOLDER
};

class TreeNodeBase {
public:
    const TreeNodeFolder* getParent() { return _parent; }
    void setParent(TreeNodeFolder* parent) { _parent = parent; }
    TreeNodeType getType() { return _type; }

private:
    TreeNodeFolder* _parent;
    TreeNodeType _type;

protected:
    TreeNodeBase(TreeNodeFolder* parent, TreeNodeType type);
};

class TreeNodeScript : public TreeNodeBase {
public:
    TreeNodeScript(const QString& filename, const QString& fullPath, ScriptOrigin origin);

    const QString& getFilename() { return _filename; };
    const QString& getFullPath() { return _fullPath; };
    const ScriptOrigin getOrigin() { return _origin; };

private:
    QString _filename;
    QString _fullPath;
    ScriptOrigin _origin;
};

class TreeNodeFolder : public TreeNodeBase {
public:
    TreeNodeFolder(const QString& foldername, TreeNodeFolder* parent);
private:
    QString _foldername;
};

class ScriptsModel : public QAbstractItemModel {
    Q_OBJECT
public:
    ScriptsModel(QObject* parent = NULL);
    QModelIndex index(int row, int column, const QModelIndex& parent) const;
    QModelIndex parent(const QModelIndex& child) const;
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;

    enum Role {
        ScriptPath = Qt::UserRole,
    };

protected slots:
    void updateScriptsLocation(const QString& newPath);
    void downloadFinished();
    void reloadLocalFiles();
    void reloadRemoteFiles();

protected:
    void requestRemoteFiles(QString marker = QString());
    bool parseXML(QByteArray xmlFile);
    void rebuildTree();

private:
    bool _loadingScripts;
    QDir _localDirectory;
    QFileSystemWatcher _fsWatcher;
    QList<TreeNodeBase*> _treeNodes;
};

#endif // hifi_ScriptsModel_h
