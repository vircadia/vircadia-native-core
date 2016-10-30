//
//  FileScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Elisa Lupin-Jimenez on 6/28/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QDebug>
#include <QBuffer>
#include <QTextCodec>
#include <QIODevice>
#include <QUrl>
#include <QByteArray>
#include <QString>
#include <QFileInfo>
#include <quazip5/quazip.h>
#include <quazip5/JlCompress.h>
#include "ResourceManager.h"

#include "FileScriptingInterface.h"


FileScriptingInterface::FileScriptingInterface(QObject* parent) : QObject(parent) {
    // nothing for now
}

void FileScriptingInterface::runUnzip(QString path, QUrl url, bool autoAdd) {
    qDebug() << "Url that was downloaded: " + url.toString();
    qDebug() << "Path where download is saved: " + path;
    QString fileName = "/" + path.section("/", -1);
    QString tempDir = path;
    tempDir.remove(fileName);
    qDebug() << "Temporary directory at: " + tempDir;
    if (!isTempDir(tempDir)) {
        qDebug() << "Temporary directory mismatch; risk of losing files";
        return;
    }

    QString file = unzipFile(path, tempDir);
    if (file != "") {
        qDebug() << "Object file to upload: " + file;
        QUrl url = QUrl::fromLocalFile(file);
        emit unzipSuccess(url.toString(), autoAdd);
    } else {
        qDebug() << "unzip failed";
    }
    qDebug() << "Removing temporary directory at: " + tempDir;
    QDir(tempDir).removeRecursively();
}

// fix to check that we are only referring to a temporary directory
bool FileScriptingInterface::isTempDir(QString tempDir) {
    QString folderName = "/" + tempDir.section("/", -1);
    QString tempContainer = tempDir;
    tempContainer.remove(folderName);
    QTemporaryDir test;
    QString testDir = test.path();
    folderName = "/" + testDir.section("/", -1);
    QString testContainer = testDir;
    testContainer.remove(folderName);
    return (testContainer == tempContainer);
}

// checks whether the webview is displaying a Clara.io page for Marketplaces.qml
bool FileScriptingInterface::isClaraLink(QUrl url) {
    return (url.toString().contains("clara.io") && !url.toString().contains("clara.io/signup"));
}

bool FileScriptingInterface::isZippedFbx(QUrl url) {
    return (url.toString().endsWith("fbx.zip"));
}

// checks whether a user tries to download a file that is not in .fbx format
bool FileScriptingInterface::isZipped(QUrl url) {
    return (url.toString().endsWith(".zip"));
}

// this function is not in use
QString FileScriptingInterface::getTempDir() {
    QTemporaryDir dir;
    dir.setAutoRemove(false);
    return dir.path();
    // do something to delete this temp dir later
}

QString FileScriptingInterface::convertUrlToPath(QUrl url) {
    QString newUrl;
    QString oldUrl = url.toString();
    newUrl = oldUrl.section("filename=", 1, 1);
    qDebug() << "Filename should be: " + newUrl;
    return newUrl;
}

// this function is not in use
void FileScriptingInterface::downloadZip(QString path, const QString link) {
    QUrl url = QUrl(link);
    auto request = ResourceManager::createResourceRequest(nullptr, url);
    connect(request, &ResourceRequest::finished, this, [this, path]{
        unzipFile(path, ""); // so intellisense isn't mad
    });
    request->send();
}


QString FileScriptingInterface::unzipFile(QString path, QString tempDir) {

    QDir dir(path);
    QString dirName = dir.path();
    QString target = tempDir + "/model_repo";
    QStringList list = JlCompress::extractDir(dirName, target);

    qDebug() << list;

    if (!list.isEmpty()) {
        return list.front();
    } else {
        qDebug() << "Extraction failed";
        return "";
    }

}

// this function is not in use
void FileScriptingInterface::recursiveFileScan(QFileInfo file, QString* dirName) {
    /*if (!file.isDir()) {
        qDebug() << "Regular file logged: " + file.fileName();
        return;
    }*/
    QFileInfoList files;

    if (file.fileName().contains(".zip")) {
        qDebug() << "Extracting archive: " + file.fileName();
        JlCompress::extractDir(file.fileName());
    }
    files = file.dir().entryInfoList();

    /*if (files.empty()) {
        files = JlCompress::getFileList(file.fileName());
    }*/

    foreach (QFileInfo file, files) {
        qDebug() << "Looking into file: " + file.fileName();
        recursiveFileScan(file, dirName);
    }
    return;
}
