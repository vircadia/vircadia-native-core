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

void FileScriptingInterface::runUnzip(QString path, QUrl url) {
	qDebug() << "Url that was downloaded: " + url.toString();
	qDebug() << "Path where download is saved: " + path;
	QString file = unzipFile(path);
	if (file != "") {
		qDebug() << "file to upload: " + file;
		QUrl url = QUrl::fromLocalFile(file);
		qDebug() << "url from local file: " + url.toString();
		emit unzipSuccess(url.toString());
		//Application::toggleAssetServerWidget(file);
	} else {
		qDebug() << "unzip failed";
	}
}

bool FileScriptingInterface::testUrl(QUrl url) {
	if (url.toString().contains(".zip") && url.toString().contains("fbx")) return true;
	qDebug() << "This model is not a .fbx packaged in a .zip. Please try with another model.";
	return false;
}

QString FileScriptingInterface::getTempDir() {
	QTemporaryDir dir;
	dir.setAutoRemove(false);
	return dir.path();
	// remember I must do something to delete this temp dir later
}

QString FileScriptingInterface::convertUrlToPath(QUrl url) {
	QString newUrl;
	QString oldUrl = url.toString();
	newUrl = oldUrl.section("filename=", 1, 1);
	qDebug() << "Filename should be: " + newUrl;
	return newUrl;
}

void FileScriptingInterface::downloadZip(QString path, const QString link) {
	QUrl url = QUrl(link);
	auto request = ResourceManager::createResourceRequest(nullptr, url);
	connect(request, &ResourceRequest::finished, this, [this, path]{
		unzipFile(path);
	});
    request->send();
}

// clement's help :D
QString FileScriptingInterface::unzipFile(QString path) {

    qDebug() << "Zip file was downloaded";
	QDir dir(path);
    QString dirName = dir.path();
	qDebug() << "Zip directory is stored at: " + dirName;
    QStringList list = JlCompress::extractDir(dirName, "C:/Users/elisa/Downloads/test");

	qDebug() << list;

	if (!list.isEmpty()) {
		return list.front();
	} else {
		qDebug() << "Extraction failed";
		return "";
	}

}

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
