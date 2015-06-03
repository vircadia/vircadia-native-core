//
//  AutoUpdate.h
//  libraries/auto-update/src
//
//  Created by Leonardo Murillo on 6/1/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AutoUpdate__
#define __hifi__AutoUpdate__

#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QDebug>
#include <QString>
#include <QUrl>
#include <QMap>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkConfiguration>
#include <QNetworkAccessManager>
#include <QXmlStreamReader>
#include <QXmlStreamAttributes>


const QUrl BUILDS_XML_URL("https://highfidelity.com/builds.xml");

class AutoUpdate : public QObject {
    Q_OBJECT
public:
    // Methods
    AutoUpdate();
    ~AutoUpdate();
    
    void checkForUpdate();
    
public slots:
    
private:
    // Members
    QMap<int, QMap<QString, QString>> _builds;
    QString _operatingSystem;
    
    // Methods
    void getLatestVersionData();
    void performAutoUpdate();
    void downloadUpdateVersion();
    QMap<int, QMap<QString, QString>> getBuildData() { return _builds; }
    void appendBuildData(int versionNumber,
                         QString downloadURL,
                         QString releaseTime,
                         QString releaseNotes,
                         QString pullRequestNumber);
    
private slots:
    void parseLatestVersionData();
    void debugBuildData();

signals:
    void latestVersionDataParsed();
    
};

#endif /* defined(__hifi__AutoUpdate__) */
