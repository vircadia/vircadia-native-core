//
//  FileScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 6/28/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTemporaryDir>
#include <QFile>
#include <QDebug>
#include <QBuffer>
#include <QUrl>
#include <QByteArray>
#include <QString>
#include <QFileInfo>
#include <quazip/quazip.h>
#include <quazip/JlCompress.h>
#include "ResourceManager.h"

#include "FileScriptingInterface.h"


FileScriptingInterface::FileScriptingInterface(QObject* parent) : QObject(parent) {
	// nothing for now
}

void FileScriptingInterface::downloadZip() {
	QUrl url(*parent);
	auto request = ResourceManager::createResourceRequest(nullptr, url);
    connect(request, &ResourceRequest::finished, this, &FileScriptingInterface::unzipFile);
    request->send();
}

// clement's help :D
void FileScriptingInterface::unzipFile() {
	ResourceRequest* request = qobject_cast<ResourceRequest*>(sender());
    QUrl url = request->getUrl();

    if (request->getResult() == ResourceRequest::Success) {
        qDebug() << "Zip file was downloaded";
	    QTemporaryDir dir;
        QByteArray compressedFileContent = request->getData(); // <-- Downloaded file is in here
        QBuffer buffer(&compressedFileContent);
        buffer.open(QIODevice::ReadOnly);

        QString dirName = dir.path();
        JlCompress::extractDir(buffer, dirName);

        QFileInfoList files = dir.entryInfoList();
        foreach (QFileInfo file, files) {
        	recursiveFileScan(file, &dirName);
        }


        /*foreach (QFileInfo file, files) {
        	if (file.isDir()) {
        		if (file.fileName().contains(".zip")) {
        			qDebug() << "Zip file expanded: " + file.fileName();
        		}

        		qDebug() << "Regular file logged: " + file.fileName();
        	}
        }*/


        //QString zipFileName = QFile::decodeName(&compressedFileContent);

        //QFile file = 
        //need to convert this byte array to a file
        /*QuaZip zip(zipFileName);

        if (zip.open(QuaZip::mdUnzip)) {
        qDebug() << "Opened";
 
        for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile()) {
            // do something
            qDebug() << zip.getCurrentFileName();
        }
        if (zip.getZipError() == UNZ_OK) {
            // ok, there was no error
        }*/


        buffer.close();
    } else {
        qDebug() << "Could not download the zip file";
    }

}

void FileScriptingInterface::recursiveFileScan(QFileInfo file, QString* dirName) {
	if (!file.isDir()) {
		qDebug() << "Regular file logged: " + file.fileName();
		return;
	}
    QFileInfoList files;

	if (file.fileName().contains(".zip")) {
		JlCompress::extractDir(file);
        qDebug() << "Extracting archive: " + file.fileName();
    }
    files = file.entryInfoList();

	/*if (files.empty()) {
		files = JlCompress::getFileList(file.fileName());
	}*/

	foreach (QFileInfo file, files) {
        qDebug() << "Looking into file: " + file.fileName();
        recursiveFileScan(file, &dirName);
	}
    return;
}
