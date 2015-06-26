//
//  Tooltip.cpp
//  libraries/ui/src
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Tooltip.h"

#include <QtCore/QUuid>

HIFI_QML_DEF(Tooltip)

Tooltip::Tooltip(QQuickItem* parent) : QQuickItem(parent) {

}

Tooltip::~Tooltip() {

}

const QString& Tooltip::getTitle() const {
    return _title;
}

const QString& Tooltip::getDescription() const {
    return _description;
}

void Tooltip::setTitle(const QString& title) {
    if (title != _title) {
        _title = title;
        emit titleChanged();
    }
}

void Tooltip::setDescription(const QString& description) {
    if (description != _description) {
        _description = description;
        emit descriptionChanged();
    }
}

void Tooltip::setVisible(bool visible) {
    QQuickItem::setVisible(visible);
}

QString Tooltip::showTip(const QString& title, const QString& description) {
    const QString newTipId = QUuid().createUuid().toString();

    Tooltip::show([&](QQmlContext*, QObject* object) {
        object->setObjectName(newTipId);
        object->setProperty("title", title);
        object->setProperty("description", description);
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
