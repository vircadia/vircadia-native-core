//
// ImageProvider.h
//
// Created by David Kelly on 2017/08/23
// Copyright 2017 High Fidelity, Inc.

// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_ImageProvider_h
#define hifi_ImageProvider_h

#include <QQuickImageProvider>
#include <QReadWriteLock>

class ImageProvider: public QQuickImageProvider {
public:
    static const QString PROVIDER_NAME;

    ImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    virtual ~ImageProvider();
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

    void setSecurityImage(const QPixmap* pixmap);
protected:
    static QReadWriteLock _rwLock;
    static QPixmap* _securityImage;

};

#endif //hifi_ImageProvider_h

