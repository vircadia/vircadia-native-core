//
// SecurityImageProvider.h
//
// Created by David Kelly on 2017/08/23
// Copyright 2017 High Fidelity, Inc.

// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_SecurityImageProvider_h
#define hifi_SecurityImageProvider_h

#include <QQuickImageProvider>
#include <QReadWriteLock>

class SecurityImageProvider: public QQuickImageProvider {
public:
    static const QString PROVIDER_NAME;

    SecurityImageProvider() : QQuickImageProvider(QQuickImageProvider::Pixmap) {}
    virtual ~SecurityImageProvider();
    QPixmap requestPixmap(const QString& id, QSize* size, const QSize& requestedSize) override;

    void setSecurityImage(const QPixmap* pixmap);
protected:
    QReadWriteLock _rwLock;
    QPixmap _securityImage;
};

#endif //hifi_SecurityImageProvider_h

