//
//  ImageProvider.cpp
//  interface/src/ui
//
//  Created by David Kelly on 8/23/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ImageProvider.h"

#include <QReadLocker>
#include <QWriteLocker>

const QString ImageProvider::PROVIDER_NAME = "security";
QReadWriteLock ImageProvider::_rwLock;
QPixmap* ImageProvider::_securityImage = nullptr;

ImageProvider::~ImageProvider() {
    QWriteLocker lock(&_rwLock);
    if (_securityImage) {
        delete _securityImage;
        _securityImage = nullptr;
    }
}

void ImageProvider::setSecurityImage(const QPixmap* pixmap) {
    // no need to delete old one, that is managed by the wallet
    QWriteLocker lock(&_rwLock);
    if (_securityImage) {
        delete _securityImage;
    }
    if (pixmap) {
        _securityImage = new QPixmap(*pixmap);
    } else {
        _securityImage = nullptr;
    }
}

QPixmap ImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {

    // adjust the internal pixmap to have the requested size
    QReadLocker lock(&_rwLock);
    if (id == "securityImage" && _securityImage) {
        *size = _securityImage->size();
        if (requestedSize.width() > 0 && requestedSize.height() > 0) {
            return _securityImage->scaled(requestedSize.width(), requestedSize.height(), Qt::KeepAspectRatio);
        } else {
            return _securityImage->copy();
        }
    }
    // otherwise just return a grey pixmap.  This avoids annoying error messages in qml we would get
    // when sending a 'null' pixmap (QPixmap())
    QPixmap greyPixmap(200, 200);
    greyPixmap.fill(QColor("darkGrey"));
    return greyPixmap;
}
