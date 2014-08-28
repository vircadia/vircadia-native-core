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

#include <QAbstractListModel>
#include <QDir>
#include <QNetworkReply>
#include <QFileSystemWatcher>

class ScriptItem {
public:
    ScriptItem(const QString& filename, const QString& fullPath);

    const QString& getFilename() { return _filename; };
    const QString& getFullPath() { return _fullPath; };

private:
    QString _filename;
    QString _fullPath;
};

class ScriptsModel : public QAbstractListModel {
    Q_OBJECT
public:
    ScriptsModel(QObject* parent = NULL);

    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

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

private:
    bool _loadingScripts;
    QDir _localDirectory;
    QFileSystemWatcher _fsWatcher;
    QList<ScriptItem*> _localFiles;
    QList<ScriptItem*> _remoteFiles;
};

#endif // hifi_ScriptsModel_h
