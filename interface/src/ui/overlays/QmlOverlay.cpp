//
//  Created by Bradley Austin Davis on 2016/01/27
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlOverlay.h"

#include <QQuickItem>
#include <QThread>

#include <DependencyManager.h>
#include <OffscreenUi.h>

QmlOverlay::QmlOverlay(const QUrl& url) {
    buildQmlElement(url);
}

QmlOverlay::QmlOverlay(const QUrl& url, const QmlOverlay* overlay)
    : Overlay2D(overlay) {
    buildQmlElement(url);
}

void QmlOverlay::buildQmlElement(const QUrl& url) {
    auto offscreenUI = DependencyManager::get<OffscreenUi>();
    if (!offscreenUI) {
        return;
    }

    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "buildQmlElement", Q_ARG(QUrl, url));
        return;
    }

    offscreenUI->load(url, [=](QQmlContext* context, QObject* object) {
        _qmlElement = dynamic_cast<QQuickItem*>(object);
        connect(_qmlElement, &QObject::destroyed, this, &QmlOverlay::qmlElementDestroyed);
    });
}

void QmlOverlay::qmlElementDestroyed() {
    _qmlElement = nullptr;
}

QmlOverlay::~QmlOverlay() {
    if (_qmlElement) {
        _qmlElement->deleteLater();
    }
    _qmlElement = nullptr;
}

// QmlOverlay replaces Overlay's properties with those defined in the QML file used but keeps Overlay2D's properties.
void QmlOverlay::setProperties(const QVariantMap& properties) {
    Overlay2D::setProperties(properties);

    // check to see if qmlElement still exists
    if (_qmlElement) {
        _qmlElement->setX(_bounds.left());
        _qmlElement->setY(_bounds.top());
        _qmlElement->setWidth(_bounds.width());
        _qmlElement->setHeight(_bounds.height());
        _qmlElement->setVisible(_visible);
        QMetaObject::invokeMethod(_qmlElement, "updatePropertiesFromScript", Qt::DirectConnection, Q_ARG(QVariant, properties));
    }
}