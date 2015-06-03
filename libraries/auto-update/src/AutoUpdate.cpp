//
//  AutoUpdate.cpp
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

#include <NetworkAccessManager.h>
#include <SharedUtil.h>
#include "AutoUpdate.h"

AutoUpdate::AutoUpdate() {
#ifdef Q_OS_WIN32
    _operatingSystem = "windows";
#endif
#ifdef Q_OS_MAC
    _operatingSystem = "mac";
#endif
#ifdef Q_OS_LINUX
    _operatingSystem = "ubuntu";
#endif
    //connect(this, SIGNAL(latestVersionDataParsed()), this, SLOT(debugBuildData()));
}

AutoUpdate::~AutoUpdate() {
    qDebug() << "[LEOTEST] The object is now destroyed";
}

void AutoUpdate::checkForUpdate() {
    this->getLatestVersionData();
    
}

void AutoUpdate::getLatestVersionData() {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest latestVersionRequest(BUILDS_XML_URL);
    latestVersionRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(latestVersionRequest);
    connect(reply, SIGNAL(finished()), this, SLOT(parseLatestVersionData()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleError(QNetworkReply::NetworkError)));
}


void AutoUpdate::parseLatestVersionData() {
    QNetworkReply* sender = qobject_cast<QNetworkReply*>(QObject::sender());
    
    QXmlStreamReader xml(sender);
    
    int version;
    QString downloadUrl;
    QString releaseTime;
    QString releaseNotes;
    QString commitSha;
    QString pullRequestNumber;
    
    
    while (!xml.atEnd() && !xml.hasError()) {
        if (xml.name().toString() == "project" &&
            xml.attributes().hasAttribute("name") &&
            xml.attributes().value("name").toString() == "interface") {
            xml.readNext();
            
            while (!xml.atEnd() && !xml.hasError() && xml.name().toString() != "project") {
                if (xml.name().toString() == "platform" &&
                    xml.attributes().hasAttribute("name") &&
                    xml.attributes().value("name").toString() == _operatingSystem) {
                    xml.readNext();
                    while (!xml.atEnd() && !xml.hasError() &&
                           xml.name().toString() != "platform") {
                        
                        if (xml.name().toString() == "build" && xml.tokenType() != QXmlStreamReader::EndElement) {
                            xml.readNext();
                            version = xml.readElementText().toInt();
                            xml.readNext();
                            downloadUrl = xml.readElementText();
                            xml.readNext();
                            releaseTime = xml.readElementText();
                            xml.readNext();
                            if (xml.name().toString() == "notes" && xml.tokenType() != QXmlStreamReader::EndElement) {
                                xml.readNext();
                                while (!xml.atEnd() && !xml.hasError() && xml.name().toString() != "notes") {
                                    if (xml.name().toString() == "note" && xml.tokenType() != QXmlStreamReader::EndElement) {
                                        releaseNotes = releaseNotes + "\n" + xml.readElementText();
                                    }
                                    xml.readNext();
                                }
                            }
                            xml.readNext();
                            commitSha = xml.readElementText();
                            xml.readNext();
                            pullRequestNumber = xml.readElementText();
                            appendBuildData(version, downloadUrl, releaseTime, releaseNotes, pullRequestNumber);
                        }
                        
                        xml.readNext();
                    }
                }
                xml.readNext();
            }
            
        } else {
            xml.readNext();
        }
    }
    sender->deleteLater();
    emit latestVersionDataParsed();
}

void AutoUpdate::debugBuildData() {
    qDebug() << "[LEOTEST] We finished parsing the xml build data";
    foreach (int key, _builds.keys()) {
        qDebug() << "[LEOTEST] Build number: " << QString::number(key);
        foreach (QString detailsKey, _builds[key].keys()) {
            qDebug() << "[LEOTEST] Key: " << detailsKey << " Value: " << _builds[key][detailsKey];
        }
    }
}

void AutoUpdate::performAutoUpdate() {
    
}

void AutoUpdate::downloadUpdateVersion() {
    
}

void AutoUpdate::appendBuildData(int versionNumber, QString downloadURL, QString releaseTime, QString releaseNotes, QString pullRequestNumber) {
    QMap<QString, QString> thisBuildDetails;
    thisBuildDetails.insert("downloadUrl", downloadURL);
    thisBuildDetails.insert("releaseTime", releaseTime);
    thisBuildDetails.insert("releaseNotes", releaseNotes);
    thisBuildDetails.insert("pullRequestNumber", pullRequestNumber);
    _builds.insert(versionNumber, thisBuildDetails);
}