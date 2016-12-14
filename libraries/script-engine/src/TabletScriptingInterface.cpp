//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TabletScriptingInterface.h"

#include <QtCore/QThread>

QObject* TabletScriptingInterface::getTablet(const QString& tabletId) {

    std::lock_guard<std::mutex> guard(_tabletProxiesMutex);

    // look up tabletId in the map.
    auto iter = _tabletProxies.find(tabletId);
    if (iter != _tabletProxies.end()) {
        // tablet already exists, just return it.
        return iter->second.data();
    } else {
        // allocate a new tablet, add it to the map then return it.
        auto tabletProxy = QSharedPointer<TabletProxy>(new TabletProxy(tabletId));
        _tabletProxies[tabletId] = tabletProxy;
        return tabletProxy.data();
    }
}

void TabletScriptingInterface::setupTablet(QString tabletId, QQuickItem* qmlTablet) {
    // AJT: TODO
    qDebug() << "AJT: setupTablet objname = " << qmlTablet->objectName();
}

//
// TabletProxy
//

TabletProxy::TabletProxy(QString name) : _name(name) {
    ;
}

QObject* TabletProxy::addButton(const QVariant& properties) {
    auto tabletButtonProxy = QSharedPointer<TabletButtonProxy>(new TabletButtonProxy(properties.toMap()));
    std::lock_guard<std::mutex> guard(_tabletButtonProxiesMutex);
    _tabletButtonProxies.push_back(tabletButtonProxy);
    return tabletButtonProxy.data();
}

void TabletProxy::removeButton(QObject* tabletButtonProxy) {
    std::lock_guard<std::mutex> guard(_tabletButtonProxiesMutex);
    auto iter = std::find(_tabletButtonProxies.begin(), _tabletButtonProxies.end(), tabletButtonProxy);
    if (iter != _tabletButtonProxies.end()) {
        _tabletButtonProxies.erase(iter);
    } else {
        qWarning() << "TabletProxy::removeButton() could not find button " << tabletButtonProxy;
    }
}

//
// TabletButtonProxy
//

TabletButtonProxy::TabletButtonProxy(const QVariantMap& properties) : _properties(properties) {
    ;
}

void TabletButtonProxy::setInitRequestHandler(const QScriptValue& handler) {
    _initRequestHandler = handler;
}

static QString IMAGE_URL_KEY = "imageUrl";
static QString IMAGE_URL_DEFAULT = "";

QString TabletButtonProxy::getImageUrl() const {
    std::lock_guard<std::mutex> guard(_propertiesMutex);
    return _properties.value(IMAGE_URL_KEY, IMAGE_URL_DEFAULT).toString();
}

void TabletButtonProxy::setImageUrl(QString imageUrl) {
    std::lock_guard<std::mutex> guard(_propertiesMutex);
    _properties[IMAGE_URL_KEY] = imageUrl;
}

#include "TabletScriptingInterface.moc"
