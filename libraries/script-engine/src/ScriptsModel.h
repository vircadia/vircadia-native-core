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

/**jsdoc
 * <p>Provided as a property of {@link ScriptDiscoveryService}.</p>
 * <p>Has properties and functions below in addition to those of <a href="http://doc.qt.io/qt-5/qabstractitemmodel.html">
 * http://doc.qt.io/qt-5/qabstractitemmodel.html</a>.</p>
 * @class ScriptsModel
 *
 * @hifi-interface
 * @hifi-client-entity
 */
class ScriptsModel : public QAbstractItemModel {
    Q_OBJECT
public:
    ScriptsModel(QObject* parent = NULL);
    ~ScriptsModel();

    /**jsdoc
     * @function ScriptsModel.index
     * @param {number} row
     * @param {number} column
     * @param {QModelIndex} parent
     * @returns {QModelIndex}
     */
    QModelIndex index(int row, int column, const QModelIndex& parent) const override;

    /**jsdoc
     * @function ScriptsModel.parent
     * @param {QModelIndex} child
     * @returns {QModelIndex}
     */
    QModelIndex parent(const QModelIndex& child) const override;

    /**jsdoc
     * @function ScriptsModel.data
     * @param {QModelIndex} index
     * @param {number} [role=0]
     * returns {string}
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**jsdoc
     * @function ScriptsModel.rowCount
     * @param {QmodelIndex} [parent=null]
     * @returns {number}
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**jsdoc
     * @function ScriptsModel.columnCount
     * @param {QmodelIndex} [parent=null]
     * @returns {number}
     */
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    /**jsdoc
     * @function ScriptsModel.getTreeNodeFromIndex
     * @param {QmodelIndex} index
     * @returns {TreeNodeBase}
     */
    TreeNodeBase* getTreeNodeFromIndex(const QModelIndex& index) const;

    /**jsdoc
     * @function ScriptsModel.getFolderNodes
     * @param {TreeNodeFolder} parent
     * @returns {TreeNodeBase[]}
     */
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
