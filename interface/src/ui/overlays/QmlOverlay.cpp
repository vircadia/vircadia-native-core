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
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    offscreenUi->returnFromUiThread([=] {
        offscreenUi->load(url, [=](QQmlContext* context, QObject* object) {
            QQuickItem* rawPtr = dynamic_cast<QQuickItem*>(object);
            // Create a shared ptr with a custom deleter lambda, that calls deleteLater
            _qmlElement = std::shared_ptr<QQuickItem>(rawPtr, [](QQuickItem* ptr) {
                if (ptr) {
                    ptr->deleteLater();
                }
            });
        });
        while (!_qmlElement) {
            qApp->processEvents();
        }
        return QVariant();
    });
}

QmlOverlay::~QmlOverlay() {
    if (_qmlElement) {
        _qmlElement->deleteLater();
        _qmlElement = nullptr;
    }
}

void QmlOverlay::setProperties(const QVariantMap& properties) {
    Overlay2D::setProperties(properties);
    auto bounds = _bounds;
    std::weak_ptr<QQuickItem> weakQmlElement;
    DependencyManager::get<OffscreenUi>()->executeOnUiThread([=] {
        // check to see if qmlElement still exists
        auto qmlElement = weakQmlElement.lock();
        if (qmlElement) {
            _qmlElement->setX(bounds.left());
            _qmlElement->setY(bounds.top());
            _qmlElement->setWidth(bounds.width());
            _qmlElement->setHeight(bounds.height());
        }
    });
    QMetaObject::invokeMethod(_qmlElement.get(), "updatePropertiesFromScript", Q_ARG(QVariant, properties));
}

void QmlOverlay::render(RenderArgs* args) {
    if (!_qmlElement) {
        return;
    }

    if (_visible != _qmlElement->isVisible()) {
        _qmlElement->setVisible(_visible);
    }
}
