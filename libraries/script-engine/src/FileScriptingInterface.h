//
//  FileScriptingInterface.h
//  libraries/script-engine/src
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
#include <QString>

class FileScriptingInterface : public QObject {
    Q_OBJECT

public:
    FileScriptingInterface(QObject* parent);

    //void runUnzip(QString path, QString importURL);
    QString getTempDir();


public slots:
    //void unzipFile(QString path);
    bool testUrl(QUrl url);
    QString convertUrlToPath(QUrl url);
    void runUnzip(QString path, QUrl url);

signals:
    void unzipSuccess(QString url);

private:
    //void downloadZip();
    QString unzipFile(QString path);
    void recursiveFileScan(QFileInfo file, QString* dirName);
    void downloadZip(QString path, const QString link);

};

#endif // hifi_FileScriptingInterface_h