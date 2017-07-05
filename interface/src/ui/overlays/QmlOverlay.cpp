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
        QQuickItem* rawPtr = dynamic_cast<QQuickItem*>(object);
        // Create a shared ptr with a custom deleter lambda, that calls deleteLater
        _qmlElement = std::shared_ptr<QQuickItem>(rawPtr, [](QQuickItem* ptr) {
            if (ptr) {
                ptr->deleteLater();
            }
        });
    });
}

QmlOverlay::~QmlOverlay() {
    _qmlElement.reset();
}

void QmlOverlay::setProperties(const QVariantMap& properties) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setProperties", Q_ARG(QVariantMap, properties));
        return;
    }

    Overlay2D::setProperties(properties);
    auto bounds = _bounds;
    std::weak_ptr<QQuickItem> weakQmlElement = _qmlElement;
    // check to see if qmlElement still exists
    auto qmlElement = weakQmlElement.lock();
    if (qmlElement) {
        qmlElement->setX(bounds.left());
        qmlElement->setY(bounds.top());
        qmlElement->setWidth(bounds.width());
        qmlElement->setHeight(bounds.height());
        QMetaObject::invokeMethod(qmlElement.get(), "updatePropertiesFromScript", Qt::DirectConnection, Q_ARG(QVariant, properties));
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
