//
//  ScriptsModel.cpp
//  interface/src
//
//  Created by Ryan Huffman on 05/12/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  S3 request code written with ModelBrowser as a reference.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <typeinfo>
#include <QUrl>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <NetworkAccessManager.h>

#include "Menu.h"

#include "ScriptsModel.h"

static const QString S3_URL = "http://s3.amazonaws.com/hifi-public";
static const QString PUBLIC_URL = "http://public.highfidelity.io";
static const QString MODELS_LOCATION = "scripts/";

static const QString PREFIX_PARAMETER_NAME = "prefix";
static const QString MARKER_PARAMETER_NAME = "marker";
static const QString IS_TRUNCATED_NAME = "IsTruncated";
static const QString CONTAINER_NAME = "Contents";
static const QString KEY_NAME = "Key";

TreeNodeBase::TreeNodeBase(TreeNodeFolder* parent, TreeNodeType type) :
    _parent(parent),
    _type(type) {
};

TreeNodeScript::TreeNodeScript(const QString& filename, const QString& fullPath, ScriptOrigin origin) :
    TreeNodeBase(NULL, TREE_NODE_TYPE_SCRIPT),
    _filename(filename),
    _fullPath(fullPath),
    _origin(origin)
{
};

TreeNodeFolder::TreeNodeFolder(const QString& foldername, TreeNodeFolder* parent) :
    TreeNodeBase(parent, TREE_NODE_TYPE_FOLDER),
    _foldername(foldername) {
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

    updateScriptsLocation(Menu::getInstance()->getScriptsLocation());
    
    connect(&_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &ScriptsModel::reloadLocalFiles);
    connect(Menu::getInstance(), &Menu::scriptLocationChanged, this, &ScriptsModel::updateScriptsLocation);

    reloadLocalFiles();
    reloadRemoteFiles();
}

QModelIndex ScriptsModel::index(int row, int column, const QModelIndex& parent) const {
    return QModelIndex();
}

QModelIndex ScriptsModel::parent(const QModelIndex& child) const {
    return QModelIndex();
}

QVariant ScriptsModel::data(const QModelIndex& index, int role) const {
    /*const QList<TreeNodeScript*>* files = NULL;
    int row = 0;
    bool isLocal = index.row() < _localFiles.size();
    if (isLocal) {
        files = &_localFiles;
        row = index.row();
    } else {
        files = &_remoteFiles;
        row = index.row() - _localFiles.size();
    }

    if (role == Qt::DisplayRole) {
        return QVariant((*files)[row]->getFilename() + (isLocal ? " (local)" : ""));
    } else if (role == ScriptPath) {
        return QVariant((*files)[row]->getFullPath());
    }*/
    return QVariant();
}

int ScriptsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return 0;// _localFiles.length() + _remoteFiles.length();
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
            if (typeid(*node) == typeid(TreeNodeScript) && static_cast<TreeNodeScript*>(node)->getOrigin() == SCRIPT_ORIGIN_REMOTE) {
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
            qDebug() << "Error: Received no data when loading remote scripts";
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
        qDebug() << "Error loading remote scripts: " << xml.errorString();
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
        qDebug() << "deleting local " << i << " " << typeid(*node).name();
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
        qDebug() << "blup" << i << typeid(*node).name();
        if (typeid(*node) == typeid(TreeNodeScript)) {
            TreeNodeScript* script = static_cast<TreeNodeScript*>(node);
            TreeNodeFolder* parent = NULL;
            QString hash;
            QStringList pathList = script->getFilename().split(tr("/"));
            QStringList::const_iterator pathIterator;
            for (pathIterator = pathList.constBegin(); pathIterator != pathList.constEnd(); ++pathIterator) {
                hash.append("/" + *pathIterator);
                qDebug() << hash;
            }
        }
    }
    folders.clear();
}
