//
//  FileScriptingInterface.h
//  interface/src/scripting
//
//  Created by Elisa Lupin-Jimenez on 6/28/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_FileScriptingInterface_h
#define hifi_FileScriptingInterface_h

#include <QtCore/QObject>
#include <QFileInfo>

class FileScriptingInterface : public QObject {
    Q_OBJECT

public:
    FileScriptingInterface(QObject* parent);

public slots:
	void unzipFile();

signals:
	void downloadZip();

private:
	//void downloadZip();
	//void unzipFile();
	void recursiveFileScan(QFileInfo file, QString* dirName);

};

#endif // hifi_FileScriptingInterface_h