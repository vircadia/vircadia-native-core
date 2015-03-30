//
//  Script.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 2015-03-30
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/*
#include <stdint.h>

#include <glm/glm.hpp>

#include <QDataStream>
#include <QtCore/QDebug>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <qendian.h>

#include <LimitedNodeList.h>
#include <NetworkAccessManager.h>
#include <SharedUtil.h>

#include "AudioRingBuffer.h"
#include "AudioFormat.h"
#include "AudioBuffer.h"
#include "AudioEditBuffer.h"
*/

#include "Script.h"

Script::Script(const QUrl& url) :
    Resource(url),
    _isReady(false)
{
    
}

void Script::downloadFinished(QNetworkReply* reply) {
    // replace our byte array with the downloaded data
    _contents = reply->readAll();
    qDebug() << "Script downloaded from:" << getURL();
    _isReady = true;
    reply->deleteLater();
}

