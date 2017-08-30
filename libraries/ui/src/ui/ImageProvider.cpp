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

void ImageProvider::setSecurityImage(QPixmap* pixmap) {
    // we are responsible for the pointer so no need to copy
    // but we do need to delete the old one.
    QWriteLocker lock(&_rwLock);
    // no need to delete old one, that is managed by the wallet
    _securityImage = pixmap;
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
    return QPixmap();
}
