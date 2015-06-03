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
                            QString version = xml.readElementText();
                            xml.readNext();
                            QString url = xml.readElementText();
                            qDebug() << "[LEOTEST] Release version " << version << " downloadable at: " << url;
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
}