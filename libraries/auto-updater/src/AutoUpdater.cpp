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

#include <unordered_map>

#include <ApplicationVersion.h>
#include <BuildInfo.h>
#include <NetworkAccessManager.h>
#include <NetworkingConstants.h>
#include <SharedUtil.h>

AutoUpdater::AutoUpdater() :
    _currentVersion(BuildInfo::BUILD_TYPE == BuildInfo::BuildType::Stable ? BuildInfo::VERSION : BuildInfo::BUILD_NUMBER)
{
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

    QUrl buildsURL;

    if (BuildInfo::BUILD_TYPE == BuildInfo::BuildType::Stable) {
        buildsURL = NetworkingConstants::BUILDS_XML_URL;
    } else if (BuildInfo::BUILD_TYPE == BuildInfo::BuildType::Master) {
        buildsURL = NetworkingConstants::MASTER_BUILDS_XML_URL;
    }
    
    QNetworkRequest latestVersionRequest(buildsURL);

    latestVersionRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    latestVersionRequest.setHeader(QNetworkRequest::UserAgentHeader, NetworkingConstants::VIRCADIA_USER_AGENT);
    QNetworkReply* reply = networkAccessManager.get(latestVersionRequest);
    connect(reply, &QNetworkReply::finished, this, &AutoUpdater::parseLatestVersionData);
}

void AutoUpdater::parseLatestVersionData() {
    QNetworkReply* sender = qobject_cast<QNetworkReply*>(QObject::sender());
    
    QXmlStreamReader xml(sender);

    struct InstallerURLs {
        QString full;
        QString clientOnly;
    };
    
    QString version;
    QString downloadUrl;
    QString releaseTime;
    QString releaseNotes;
    QString commitSha;
    QString pullRequestNumber;

    QString versionKey;

    // stable builds look at the stable_version node (semantic version)
    // master builds look at the version node (build number)
    if (BuildInfo::BUILD_TYPE == BuildInfo::BuildType::Stable) {
        versionKey = "stable_version";
    } else if (BuildInfo::BUILD_TYPE == BuildInfo::BuildType::Master) {
        versionKey = "version";
    }
    
    while (xml.readNextStartElement()) {
        if (xml.name() == "projects") {
            while (xml.readNextStartElement()) {
                if (xml.name().toString() == "project" &&
                    xml.attributes().hasAttribute("name") &&
                    xml.attributes().value("name").toString() == "interface") {

                    while (xml.readNextStartElement()) {

                        if (xml.name().toString() == "platform" &&
                            xml.attributes().hasAttribute("name") &&
                            xml.attributes().value("name").toString() == _operatingSystem) {

                            while (xml.readNextStartElement()) {
                                if (xml.name() == "build") {
                                    QHash<QString, InstallerURLs> campaignInstallers;

                                    while (xml.readNextStartElement()) {
                                        if (xml.name() == versionKey) {
                                            version = xml.readElementText();
                                        } else if (xml.name() == "url") {
                                            downloadUrl = xml.readElementText();
                                        } else if (xml.name() == "installers") {
                                            while (xml.readNextStartElement()) {
                                                QString campaign = xml.name().toString();
                                                QString full;
                                                QString clientOnly;
                                                while (xml.readNextStartElement()) {
                                                    if (xml.name() == "full") {
                                                        full = xml.readElementText();
                                                    } else if (xml.name() == "client_only") {
                                                        clientOnly = xml.readElementText();
                                                    } else {
                                                        xml.skipCurrentElement();
                                                    }
                                                }
                                                campaignInstallers[campaign] = { full, clientOnly };
                                            }
                                        } else if (xml.name() == "timestamp") {
                                            releaseTime = xml.readElementText();
                                        } else if (xml.name() == "notes") {
                                            while (xml.readNextStartElement()) {
                                                if (xml.name() == "note") {
                                                    releaseNotes = releaseNotes + "\n" + xml.readElementText();
                                                } else {
                                                    xml.skipCurrentElement();
                                                }
                                            }
                                        } else if (xml.name() == "sha") {
                                            commitSha = xml.readElementText();
                                        } else if (xml.name() == "pull_request") {
                                            pullRequestNumber = xml.readElementText();
                                        } else {
                                            xml.skipCurrentElement();
                                        }
                                    }

                                    static const QString DEFAULT_INSTALLER_CAMPAIGN_NAME = "standard";
                                    for (auto& campaign : { _installerCampaign, DEFAULT_INSTALLER_CAMPAIGN_NAME }) {
                                        auto it = campaignInstallers.find(campaign);
                                        if (it != campaignInstallers.end()) {
                                            auto& urls = *it;
                                            if (_installerType == InstallerType::CLIENT_ONLY) {
                                                if (!urls.clientOnly.isEmpty()) {
                                                    downloadUrl = urls.clientOnly;
                                                    break;
                                                }
                                            } else {
                                                if (!urls.full.isEmpty()) {
                                                    downloadUrl = urls.full;
                                                    break;
                                                }
                                            }
                                        }
                                    }

                                    appendBuildData(version, downloadUrl, releaseTime, releaseNotes, pullRequestNumber);
                                    releaseNotes = "";
                                } else {
                                    xml.skipCurrentElement();
                                }
                            }
                        } else {
                            xml.skipCurrentElement();
                        }
                    }
                } else {
                    xml.skipCurrentElement();
                }
            }
        } else {
            xml.skipCurrentElement();
        }
    }

    sender->deleteLater();
    emit latestVersionDataParsed();
}

void AutoUpdater::checkVersionAndNotify() {
    if (_builds.empty()) {
        // no build data was found for this platform
        return;
    }

    qDebug() << "Checking if update version" << _builds.lastKey().versionString
        << "is newer than current version" << _currentVersion.versionString;

    if (_builds.lastKey() > _currentVersion) {
        emit newVersionIsAvailable();
    }
}

void AutoUpdater::openLatestUpdateURL() {
    const QMap<QString, QString>& chosenVersion = _builds.last();
    const QUrl& downloadUrl = chosenVersion.value("downloadUrl");
    QDesktopServices::openUrl(downloadUrl);
    QCoreApplication::quit();
}

void AutoUpdater::downloadUpdateVersion(const QString& version) {
    emit newVersionIsDownloaded();
}

void AutoUpdater::appendBuildData(const QString& versionNumber,
                                 const QString& downloadURL,
                                 const QString& releaseTime,
                                 const QString& releaseNotes,
                                 const QString& pullRequestNumber) {
    
    QMap<QString, QString> thisBuildDetails;
    thisBuildDetails.insert("downloadUrl", downloadURL);
    thisBuildDetails.insert("releaseTime", releaseTime);
    thisBuildDetails.insert("releaseNotes", releaseNotes);
    thisBuildDetails.insert("pullRequestNumber", pullRequestNumber);
    _builds.insert(ApplicationVersion(versionNumber), thisBuildDetails);
    
}
