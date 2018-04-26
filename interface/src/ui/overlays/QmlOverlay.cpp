//
//  Created by Bradley Austin Davis on 2016/01/27
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlOverlay.h"

#include <QQuickItem>

#include <DependencyManager.h>
#include <GeometryCache.h>
#include <GLMHelpers.h>
#include <OffscreenUi.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h>
#include <TextureCache.h>
#include <ViewFrustum.h>

#include "Application.h"
#include "text/FontFamilies.h"

QmlOverlay::QmlOverlay(const QUrl& url) {
    buildQmlElement(url);
}

QmlOverlay::QmlOverlay(const QUrl& url, const QmlOverlay* textOverlay)
    : Overlay2D(textOverlay) {
    buildQmlElement(url);
}

void QmlOverlay::buildQmlElement(const QUrl& url) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "buildQmlElement", Q_ARG(QUrl, url));
        return;
    }

    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->load(url, [=](QQmlContext* context, QObject* object) {
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
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setProperties", Q_ARG(QVariantMap, properties));
        return;
    }

    Overlay2D::setProperties(properties);
    auto bounds = _bounds;
    // check to see if qmlElement still exists
    if (_qmlElement) {
        _qmlElement->setX(bounds.left());
        _qmlElement->setY(bounds.top());
        _qmlElement->setWidth(bounds.width());
        _qmlElement->setHeight(bounds.height());
        QMetaObject::invokeMethod(_qmlElement, "updatePropertiesFromScript", Qt::DirectConnection, Q_ARG(QVariant, properties));
    }
}

void QmlOverlay::render(RenderArgs* args) {
    if (!_qmlElement) {
        return;
    }

    if (_visible != _qmlElement->isVisible()) {
        _qmlElement->setVisible(_visible);
    }
}
