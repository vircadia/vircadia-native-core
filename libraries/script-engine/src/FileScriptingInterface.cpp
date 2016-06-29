//
//  FileScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 6/28/16.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QTemporaryDir>
#include <QFile>
#include <QUrl>
#include "ResourceManager.h"

#include "FileScriptingInterface.h"


FileScriptingInterface::FileScriptingInterface(QObject* parent) {
	// nothing for now
}

void FileScriptingInterface::downloadZip() {
	QUrl url(*parent);
	auto request = ResourceManager::createResourceRequest(nullptr, url);
    connect(request, &ResourceRequest::finished, this, &FileScriptingInterface::unzipFile);
    request->send();
}

// clement's help :D
bool FileScriptingInterface::unzipFile() {
	ResourceRequest* request = qobject_cast<ResourceRequest*>(sender());

        // Get the file URL
        QUrl url = request->getUrl();

        if (request->getResult() == ResourceRequest::Success) {
            qDebug() << "Success =)";

            QByteArray fileContent = request->getData(); // <-- Downloaded file is in here
            // Do stuff
            //
            // unzipFile(fileContent);

        } else {
            qDebug() << "Could not download the file =(";
        }

}

