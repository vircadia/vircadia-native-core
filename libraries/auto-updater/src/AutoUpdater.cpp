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
#include <unordered_map>

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

    struct InstallerURLs {
        QString full;
        QString clientOnly;
    };
    
    int version { 0 };
    QString downloadUrl;
    QString releaseTime;
    QString releaseNotes;
    QString commitSha;
    QString pullRequestNumber;
    
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
                                        if (xml.name() == "version") {
                                            version = xml.readElementText().toInt();
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