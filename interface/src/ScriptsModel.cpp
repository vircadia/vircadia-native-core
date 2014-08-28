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

#include <QUrl>
#include <QUrlQuery>
#include <QXmlStreamReader>

#include <NetworkAccessManager.h>

#include "Menu.h"

#include "ScriptsModel.h"

static const QString S3_URL = "http://highfidelity-public.s3-us-west-1.amazonaws.com";
static const QString PUBLIC_URL = "http://public.highfidelity.io";
static const QString MODELS_LOCATION = "scripts/";

static const QString PREFIX_PARAMETER_NAME = "prefix";
static const QString MARKER_PARAMETER_NAME = "marker";
static const QString IS_TRUNCATED_NAME = "IsTruncated";
static const QString CONTAINER_NAME = "Contents";
static const QString KEY_NAME = "Key";

ScriptItem::ScriptItem(const QString& filename, const QString& fullPath) :
    _filename(filename),
    _fullPath(fullPath) {
};

ScriptsModel::ScriptsModel(QObject* parent) :
    QAbstractListModel(parent),
    _loadingScripts(false),
    _localDirectory(),
    _fsWatcher(),
    _localFiles(),
    _remoteFiles() {

    QString scriptPath = Menu::getInstance()->getScriptsLocation();

    _localDirectory.setPath(scriptPath);
    _localDirectory.setFilter(QDir::Files | QDir::Readable);
    _localDirectory.setNameFilters(QStringList("*.js"));

    _fsWatcher.addPath(_localDirectory.absolutePath());
    connect(&_fsWatcher, &QFileSystemWatcher::directoryChanged, this, &ScriptsModel::reloadLocalFiles);

    connect(Menu::getInstance(), &Menu::scriptLocationChanged, this, &ScriptsModel::updateScriptsLocation);

    reloadLocalFiles();
    reloadRemoteFiles();
}

QVariant ScriptsModel::data(const QModelIndex& index, int role) const {
    const QList<ScriptItem*>* files = NULL;
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
    }
    return QVariant();
}

int ScriptsModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return _localFiles.length() + _remoteFiles.length();
}

void ScriptsModel::updateScriptsLocation(const QString& newPath) {
    _fsWatcher.removePath(_localDirectory.absolutePath());
    _localDirectory.setPath(newPath);
    _fsWatcher.addPath(_localDirectory.absolutePath());
    reloadLocalFiles();
}

void ScriptsModel::reloadRemoteFiles() {
    if (!_loadingScripts) {
        _loadingScripts = true;
        while (!_remoteFiles.isEmpty()) {
            delete _remoteFiles.takeFirst();
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

    NetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
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
                        _remoteFiles.append(new ScriptItem(lastKey.mid(MODELS_LOCATION.length()), S3_URL + "/" + lastKey));
                    }
                }
                xml.readNext();
            }
        }
        xml.readNext();
    }

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

    while (!_localFiles.isEmpty()) {
        delete _localFiles.takeFirst();
    }

    _localDirectory.refresh();

    const QFileInfoList localFiles = _localDirectory.entryInfoList();
    for (int i = 0; i < localFiles.size(); i++) {
        QFileInfo file = localFiles[i];
        _localFiles.append(new ScriptItem(file.fileName(), file.absoluteFilePath()));
    }

    endResetModel();
}
