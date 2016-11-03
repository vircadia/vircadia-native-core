//
//  AutoUpdater.cpp
//  libraries/auto-update/src
//
//  Created by Leonardo Murillo on 6/1/2015.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AutoUpdater.h"

#include <NetworkAccessManager.h>
#include <SharedUtil.h>

AutoUpdater::AutoUpdater() {
#if defined Q_OS_WIN32
    _operatingSystem = "windows";
#elif defined Q_OS_MAC
    _operatingSystem = "mac";
#elif defined Q_OS_LINUX
    _operatingSystem = "ubuntu";
#endif
    
    connect(this, SIGNAL(latestVersionDataParsed()), this, SLOT(checkVersionAndNotify()));
}

void AutoUpdater::checkForUpdate() {
    this->getLatestVersionData();
}

void AutoUpdater::getLatestVersionData() {
    QNetworkAccessManager& networkAccessManager = NetworkAccessManager::getInstance();
    QNetworkRequest latestVersionRequest(BUILDS_XML_URL);
    latestVersionRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    latestVersionRequest.setHeader(QNetworkRequest::UserAgentHeader, HIGH_FIDELITY_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(latestVersionRequest);
    connect(reply, &QNetworkReply::finished, this, &AutoUpdater::parseLatestVersionData);
}

void AutoUpdater::parseLatestVersionData() {
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
                            releaseNotes = "";
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

void AutoUpdater::checkVersionAndNotify() {
    if (QCoreApplication::applicationVersion() == "dev" ||
        QCoreApplication::applicationVersion().contains("PR") ||
        _builds.empty()) {
        // No version checking is required in dev builds or when no build
        // data was found for the platform
        return;
    }
    int latestVersionAvailable = _builds.lastKey();
    if (QCoreApplication::applicationVersion().toInt() < latestVersionAvailable) {
        emit newVersionIsAvailable();
    }
}

void AutoUpdater::performAutoUpdate(int version) {
    // NOTE: This is not yet auto updating - however this is a checkpoint towards that end
    // Next PR will handle the automatic download, upgrading and application restart
    const QMap<QString, QString>& chosenVersion = _builds.value(version);
    const QUrl& downloadUrl = chosenVersion.value("downloadUrl");
    QDesktopServices::openUrl(downloadUrl);
    QCoreApplication::quit();
}

void AutoUpdater::downloadUpdateVersion(int version) {
    emit newVersionIsDownloaded();
}

void AutoUpdater::appendBuildData(int versionNumber,
                                 const QString& downloadURL,
                                 const QString& releaseTime,
                                 const QString& releaseNotes,
                                 const QString& pullRequestNumber) {
    
    QMap<QString, QString> thisBuildDetails;
    thisBuildDetails.insert("downloadUrl", downloadURL);
    thisBuildDetails.insert("releaseTime", releaseTime);
    thisBuildDetails.insert("releaseNotes", releaseNotes);
    thisBuildDetails.insert("pullRequestNumber", pullRequestNumber);
    _builds.insert(versionNumber, thisBuildDetails);
    
}