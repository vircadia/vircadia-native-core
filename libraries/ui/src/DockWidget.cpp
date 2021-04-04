//
//  DockWidget.cpp
//  libraries/ui/src
//
//  Created by Dante Ruiz 05-07-2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "DockWidget.h"

#include "OffscreenUi.h"

#include <QtQml/QQmlEngine>
#include <QtQml/QQmlContext>
#include <QQuickView>
#include <gl/GLHelpers.h>

#include <PathUtils.h>

static void quickViewDeleter(QQuickView* quickView) {
    quickView->deleteLater();
}

DockWidget::DockWidget(const QString& title, QWidget* parent) : QDockWidget(title, parent) {
    if (auto offscreenUI = DependencyManager::get<OffscreenUi>()) {
        auto qmlEngine = offscreenUI->getSurfaceContext()->engine();
        _quickView = std::shared_ptr<QQuickView>(new QQuickView(qmlEngine, nullptr), quickViewDeleter);
        _quickView->setFormat(getDefaultOpenGLSurfaceFormat());
        QWidget* widget = QWidget::createWindowContainer(_quickView.get());
        setWidget(widget);
        QWidget* headerWidget = new QWidget();
        setTitleBarWidget(headerWidget);
    }
}

void DockWidget::setSource(const QUrl& url) {
    _quickView->setSource(url);
}

QQuickItem* DockWidget::getRootItem() const {
    return _quickView->rootObject();
}

std::shared_ptr<QQuickView> DockWidget::getQuickView() const {
    return _quickView;
}

void DockWidget::resizeEvent(QResizeEvent* event) {
    // This signal is currently handled in `InteractiveWindow.cpp`. There, it's used to
    // emit a `windowGeometryChanged()` signal, which is handled by scripts
    // that need to know when to change the position of their overlay UI elements.
    emit onResizeEvent();
}