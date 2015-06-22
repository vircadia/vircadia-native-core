//
//  Tooltip.cpp
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Tooltip.h"
#include <QUuid>

HIFI_QML_DEF(Tooltip)

Tooltip::Tooltip(QQuickItem* parent) : QQuickItem(parent) {
}

Tooltip::~Tooltip() {
}

QString Tooltip::text() const {
    return _text;
}

void Tooltip::setText(const QString& arg) {
    if (arg != _text) {
        _text = arg;
        emit textChanged();
    }
}

void Tooltip::setVisible(bool visible) {
    QQuickItem::setVisible(visible);
}

QString Tooltip::showTip(const QString& text) {
    const QString newTipId = QUuid().createUuid().toString();
    Tooltip::show([&](QQmlContext*, QObject* object) {
        object->setObjectName(newTipId);
        object->setProperty("text", text);
    });
    return newTipId;
}

void Tooltip::closeTip(const QString& tipId) {
    auto rootItem = DependencyManager::get<OffscreenUi>()->getRootItem();
    QQuickItem* that = rootItem->findChild<QQuickItem*>(tipId);
    if (that) {
        that->deleteLater();
    }
}