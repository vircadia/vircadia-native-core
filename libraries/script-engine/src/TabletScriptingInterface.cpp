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

    std::lock_guard<std::mutex> guard(_mutex);

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

void TabletScriptingInterface::setQmlTablet(QString tabletId, QQuickItem* qmlTablet) {
    TabletProxy* tablet = qobject_cast<TabletProxy*>(getTablet(tabletId));
    if (tablet) {
        tablet->setQmlTablet(qmlTablet);
    } else {
        qWarning() << "TabletScriptingInterface::setupTablet() bad tablet object";
    }
}

//
// TabletProxy
//

TabletProxy::TabletProxy(QString name) : _name(name) {
    ;
}

static void addButtonProxyToQmlTablet(QQuickItem* qmlTablet, TabletButtonProxy* buttonProxy) {
    QVariant resultVar;
    Qt::ConnectionType connectionType = Qt::AutoConnection;
    if (QThread::currentThread() != qmlTablet->thread()) {
        connectionType = Qt::BlockingQueuedConnection;
    }
    bool hasResult = QMetaObject::invokeMethod(qmlTablet, "addButtonProxy", connectionType,
                                               Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, buttonProxy->getProperties()));
    if (!hasResult) {
        qWarning() << "TabletScriptingInterface addButtonProxyToQmlTablet has no result";
        return;
    }

    QObject* qmlButton = qvariant_cast<QObject *>(resultVar);
    if (!qmlButton) {
        qWarning() << "TabletScriptingInterface addButtonProxyToQmlTablet result not a QObject";
        return;
    }
    QObject::connect(qmlButton, SIGNAL(clicked()), buttonProxy, SLOT(clickedSlot()));
    buttonProxy->setQmlButton(qobject_cast<QQuickItem*>(qmlButton));
}

void TabletProxy::setQmlTablet(QQuickItem* qmlTablet) {
    std::lock_guard<std::mutex> guard(_mutex);
    if (qmlTablet) {
        _qmlTablet = qmlTablet;
        for (auto& buttonProxy : _tabletButtonProxies) {
            addButtonProxyToQmlTablet(_qmlTablet, buttonProxy.data());
        }
    } else {
        for (auto& buttonProxy : _tabletButtonProxies) {
            buttonProxy->setQmlButton(nullptr);
        }
        _qmlTablet = nullptr;
    }
}

QObject* TabletProxy::addButton(const QVariant& properties) {
    auto tabletButtonProxy = QSharedPointer<TabletButtonProxy>(new TabletButtonProxy(properties.toMap()));
    std::lock_guard<std::mutex> guard(_mutex);
    _tabletButtonProxies.push_back(tabletButtonProxy);
    if (_qmlTablet) {
        addButtonProxyToQmlTablet(_qmlTablet, tabletButtonProxy.data());
    }
    return tabletButtonProxy.data();
}

void TabletProxy::removeButton(QObject* tabletButtonProxy) {
    std::lock_guard<std::mutex> guard(_mutex);
    auto iter = std::find(_tabletButtonProxies.begin(), _tabletButtonProxies.end(), tabletButtonProxy);
    if (iter != _tabletButtonProxies.end()) {
        if (_qmlTablet) {
            (*iter)->setQmlButton(nullptr);
            QMetaObject::invokeMethod(_qmlTablet, "removeButtonProxy", Qt::AutoConnection, Q_ARG(QVariant, (*iter)->getProperties()));
        }
        _tabletButtonProxies.erase(iter);
    } else {
        qWarning() << "TabletProxy::removeButton() could not find button " << tabletButtonProxy;
    }
}

//
// TabletButtonProxy
//

const QString UUID_KEY = "uuid";

TabletButtonProxy::TabletButtonProxy(const QVariantMap& properties) : _uuid(QUuid::createUuid()), _properties(properties) {
    // this is used to uniquely identify this button.
    _properties[UUID_KEY] = _uuid;
}

void TabletButtonProxy::setQmlButton(QQuickItem* qmlButton) {
    std::lock_guard<std::mutex> guard(_mutex);
    _qmlButton = qmlButton;
}

// TABLET_UI_HACK TODO: add property accessors, and forward property changes to the _qmlButton if present.

#include "TabletScriptingInterface.moc"
