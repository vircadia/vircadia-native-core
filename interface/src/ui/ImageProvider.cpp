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
#include <QDebug>

const QString ImageProvider::PROVIDER_NAME = "security";

QPixmap ImageProvider::requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) {

    // adjust the internal pixmap to have the requested size
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
