//
//  Created by Ryan Huffman on 05/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  S3 request code written with ModelBrowser as a reference.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ScriptsModel.h"

#include <QUrl>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <NetworkAccessManager.h>

#include "ScriptEngine.h"
#include "ScriptEngines.h"
#include "ScriptEngineLogging.h"
#define __STR2__(x) #x
#define __STR1__(x) __STR2__(x)
#define __LOC__ __FILE__ "("__STR1__(__LINE__)") : Warning Msg: "


static const QString S3_URL = "http://s3.amazonaws.com/hifi-public";
static const QString PUBLIC_URL = "http://public.highfidelity.io";
static const QString MODELS_LOCATION = "scripts/";

static const QString PREFIX_PARAMETER_NAME = "prefix";
static const QString MARKER_PARAMETER_NAME = "marker";
static const QString IS_TRUNCATED_NAME = "IsTruncated";
static const QString CONTAINER_NAME = "Contents";
static const QString KEY_NAME = "Key";

TreeNodeBase::TreeNodeBase(TreeNodeFolder* parent, const QString& name, TreeNodeType type) :
    _parent(parent),
    _type(type),
    _name(name) {
};

TreeNodeScript::TreeNodeScript(const QString& localPath, const QString& fullPath, ScriptOrigin origin) :
    TreeNodeBase(NULL, localPath.split("/").last(), TREE_NODE_TYPE_SCRIPT),
    _localPath(localPath),
    _fullPath(fullPath),
    _origin(origin) {
};

TreeNodeFolder::TreeNodeFolder(const QString& foldername, TreeNodeFolder* parent) :
    TreeNodeBase(parent, foldername, TREE_NODE_TYPE_FOLDER) {
};

ScriptsModel::ScriptsModel(QObject* parent) :
    QAbstractItemModel(parent),
    _loadingScripts(false),
    _localDirectory(),
    _fsWatcher(),
    _treeNodes()
{
    _localDirectory.setFilter(QDir::Files | QDir::Readable);
    _localDirectory.setNameFilters(QStringList("*.js"));

    connect(&_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &ScriptsModel::reloadLocalFiles);
    reloadLocalFiles();
    reloadRemoteFiles();
}

ScriptsModel::~ScriptsModel() {
    for (int i = _treeNodes.size() - 1; i >= 0; i--) {
        delete _treeNodes.at(i);
    }
    _treeNodes.clear();
}

TreeNodeBase* ScriptsModel::getTreeNodeFromIndex(const QModelIndex& index) const {
    if (index.isValid()) {
        return static_cast<TreeNodeBase*>(index.internalPointer());
    }
    return NULL;
}

QModelIndex ScriptsModel::index(int row, int column, const QModelIndex& parent) const {
    if (row < 0 || column < 0) {
        return QModelIndex();
    }
    return createIndex(row, column, getFolderNodes(static_cast<TreeNodeFolder*>(getTreeNodeFromIndex(parent))).at(row));
}

QModelIndex ScriptsModel::parent(const QModelIndex& child) const {
    TreeNodeFolder* parent = (static_cast<TreeNodeBase*>(child.internalPointer()))->getParent();
    if (!parent) {
        return QModelIndex();
    }
    TreeNodeFolder* grandParent = parent->getParent();
    int row = getFolderNodes(grandParent).indexOf(parent);
    return createIndex(row, 0, parent);
}

QVariant ScriptsModel::data(const QModelIndex& index, int role) const {
    TreeNodeBase* node = getTreeNodeFromIndex(index);
    if (!node) {
        return QVariant();
    }
    if (node->getType() == TREE_NODE_TYPE_SCRIPT) {
        TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
        if (role == Qt::DisplayRole) {
            return QVariant(script->getName() + (script->getOrigin() == SCRIPT_ORIGIN_LOCAL ? " (local)" : ""));
        } else if (role == ScriptPath) {
            return QVariant(script->getFullPath());
        }
    } else if (node->getType() == TREE_NODE_TYPE_FOLDER) {
        TreeNodeFolder* folder = static_cast<TreeNodeFolder*>(node);
        if (role == Qt::DisplayRole) {
            return QVariant(folder->getName());
        }
    }
    return QVariant();
}

int ScriptsModel::rowCount(const QModelIndex& parent) const {
    return getFolderNodes(static_cast<TreeNodeFolder*>(getTreeNodeFromIndex(parent))).count();
}

int ScriptsModel::columnCount(const QModelIndex& parent) const {
    return 1;
}

void ScriptsModel::updateScriptsLocation(const QString& newPath) {
    _fsWatcher.removePath(_localDirectory.absolutePath());
    
    if (!newPath.isEmpty()) {
        _localDirectory.setPath(newPath);
        
        if (!_localDirectory.absolutePath().isEmpty()) {
            _fsWatcher.addPath(_localDirectory.absolutePath());
        }
    }
    
    reloadLocalFiles();
}

void ScriptsModel::reloadRemoteFiles() {
    if (!_loadingScripts) {
        _loadingScripts = true;
        for (int i = _treeNodes.size() - 1; i >= 0; i--) {
            TreeNodeBase* node = _treeNodes.at(i);
            if (node->getType() == TREE_NODE_TYPE_SCRIPT &&
                static_cast<TreeNodeScript*>(node)->getOrigin() == SCRIPT_ORIGIN_REMOTE)
            {
                delete node;
                _treeNodes.removeAt(i);
            }
        }
        requestRemoteFiles();
    }
}

void ScriptsModel::requestRemoteFiles(QString marker) {
    QUrl url(S3_URL);
    QUrlQuery query;
    query.addQueryItem(PREFIX_PARAMETER_NAME, MODELS_LOCATION);
    if (!marker.isEmpty()) {
        query.addQueryItem(MARKER_PARAMETER_NAME, marker);
    }
    url.setQuery(query);

    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(request);
    connect(reply, SIGNAL(finished()), SLOT(downloadFinished()));
}

void ScriptsModel::downloadFinished() {
    QNetworkReply* reply = static_cast<QNetworkReply*>(sender());
    bool finished = true;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();

        if (!data.isEmpty()) {
            finished = parseXML(data);
        } else {
            qCDebug(scriptengine) << "Error: Received no data when loading remote scripts";
        }
    }

    reply->deleteLater();
    sender()->deleteLater();

    if (finished) {
        _loadingScripts = false;
    }
}

bool ScriptsModel::parseXML(QByteArray xmlFile) {
    beginResetModel();

    QXmlStreamReader xml(xmlFile);
    QRegExp jsRegex(".*\\.js");
    bool truncated = false;
    QString lastKey;

    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == IS_TRUNCATED_NAME) {
            while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == IS_TRUNCATED_NAME)) {
                xml.readNext();
                if (xml.text().toString() == "true") {
                    truncated = true;
                }
            }
        }

        if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == CONTAINER_NAME) {
            while (!(xml.tokenType() == QXmlStreamReader::EndElement && xml.name() == CONTAINER_NAME)) {
                if (xml.tokenType() == QXmlStreamReader::StartElement && xml.name() == KEY_NAME) {
                    xml.readNext();
                    lastKey = xml.text().toString();
                    if (jsRegex.exactMatch(xml.text().toString())) {
                        _treeNodes.append(new TreeNodeScript(lastKey.mid(MODELS_LOCATION.length()), S3_URL + "/" + lastKey, SCRIPT_ORIGIN_REMOTE));
                    }
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }
    rebuildTree();
    endResetModel();

    // Error handling
    if (xml.hasError()) {
        qCDebug(scriptengine) << "Error loading remote scripts: " << xml.errorString();
        return true;
    }

    if (truncated) {
        requestRemoteFiles(lastKey);
    }

    // If this request was not truncated, we are done.
    return !truncated;
}

void ScriptsModel::reloadLocalFiles() {
    beginResetModel();

    for (int i = _treeNodes.size() - 1; i >= 0; i--) {
        TreeNodeBase* node = _treeNodes.at(i);
        if (node->getType() == TREE_NODE_TYPE_SCRIPT &&
            static_cast<TreeNodeScript*>(node)->getOrigin() == SCRIPT_ORIGIN_LOCAL)
        {
            delete node;
            _treeNodes.removeAt(i);
        }
    }

    _localDirectory.refresh();

    const QFileInfoList localFiles = _localDirectory.entryInfoList();
    for (int i = 0; i < localFiles.size(); i++) {
        QFileInfo file = localFiles[i];
        _treeNodes.append(new TreeNodeScript(file.fileName(), file.absoluteFilePath(), SCRIPT_ORIGIN_LOCAL));
    }
    rebuildTree();
    endResetModel();
}

void ScriptsModel::rebuildTree() {
    for (int i = _treeNodes.size() - 1; i >= 0; i--) {
        if (_treeNodes.at(i)->getType() == TREE_NODE_TYPE_FOLDER) {
            delete _treeNodes.at(i);
            _treeNodes.removeAt(i);
        }
    }
    QHash<QString, TreeNodeFolder*> folders;
    for (int i = 0; i < _treeNodes.size(); i++) {
        TreeNodeBase* node = _treeNodes.at(i);
        if (node->getType() == TREE_NODE_TYPE_SCRIPT) {
            TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
            TreeNodeFolder* parent = NULL;
            QString hash;
            QStringList pathList = script->getLocalPath().split(tr("/"));
            pathList.removeLast();
            QStringList::const_iterator pathIterator;
            for (pathIterator = pathList.constBegin(); pathIterator != pathList.constEnd(); ++pathIterator) {
                hash.append(*pathIterator + "/");
                if (!folders.contains(hash)) {
                    folders[hash] = new TreeNodeFolder(*pathIterator, parent);
                }
                parent = folders[hash];
            }
            script->setParent(parent);
        }
    }
    QHash<QString, TreeNodeFolder*>::const_iterator folderIterator;
    for (folderIterator = folders.constBegin(); folderIterator != folders.constEnd(); ++folderIterator) {
        _treeNodes.append(*folderIterator);
    }
    folders.clear();
}

QList<TreeNodeBase*> ScriptsModel::getFolderNodes(TreeNodeFolder* parent) const {
    QList<TreeNodeBase*> result;
    for (int i = 0; i < _treeNodes.size(); i++) {
        TreeNodeBase* node = _treeNodes.at(i);
        if (node->getParent() == parent) {
            result.append(node);
        }
    }
    return result;
}
