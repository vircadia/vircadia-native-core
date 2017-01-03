//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "TabletScriptingInterface.h"

#include <QtCore/QThread>

#include "ScriptEngineLogging.h"

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

void TabletScriptingInterface::setQmlTabletRoot(QString tabletId, QQuickItem* qmlTabletRoot, QObject* qmlOffscreenSurface) {
    TabletProxy* tablet = qobject_cast<TabletProxy*>(getTablet(tabletId));
    if (tablet && qmlOffscreenSurface) {
        tablet->setQmlTabletRoot(qmlTabletRoot, qmlOffscreenSurface);
    } else {
        qCWarning(scriptengine) << "TabletScriptingInterface::setupTablet() bad tablet object";
    }
}

//
// TabletProxy
//

static const char* TABLET_SOURCE_URL = "Tablet.qml";
static const char* WEB_VIEW_SOURCE_URL = "TabletWebView.qml";
static const char* LOADER_SOURCE_PROPERTY_NAME = "loaderSource";

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
        qCWarning(scriptengine) << "TabletScriptingInterface addButtonProxyToQmlTablet has no result";
        return;
    }

    QObject* qmlButton = qvariant_cast<QObject *>(resultVar);
    if (!qmlButton) {
        qCWarning(scriptengine) << "TabletScriptingInterface addButtonProxyToQmlTablet result not a QObject";
        return;
    }
    QObject::connect(qmlButton, SIGNAL(clicked()), buttonProxy, SLOT(clickedSlot()));
    buttonProxy->setQmlButton(qobject_cast<QQuickItem*>(qmlButton));
}

void TabletProxy::setQmlTabletRoot(QQuickItem* qmlTabletRoot, QObject* qmlOffscreenSurface) {
    std::lock_guard<std::mutex> guard(_mutex);
    _qmlOffscreenSurface = qmlOffscreenSurface;
    _qmlTabletRoot = qmlTabletRoot;
    if (_qmlTabletRoot && _qmlOffscreenSurface) {
        QObject::connect(_qmlOffscreenSurface, SIGNAL(webEventReceived(QVariant)), this, SIGNAL(webEventReceived(QVariant)));
        gotoHomeScreen();
    } else {
        removeButtonsFromHomeScreen();
    }
}

void TabletProxy::gotoHomeScreen() {
    if (_qmlTabletRoot) {
        QString tabletSource = _qmlTabletRoot->property(LOADER_SOURCE_PROPERTY_NAME).toString();
        if (tabletSource != TABLET_SOURCE_URL) {
            QMetaObject::invokeMethod(_qmlTabletRoot, "loadSource", Q_ARG(const QVariant&, QVariant(TABLET_SOURCE_URL)));
            addButtonsToHomeScreen();
        }
    }
}

void TabletProxy::gotoWebScreen(const QString& url) {
    if (_qmlTabletRoot) {
        removeButtonsFromHomeScreen();
        QString tabletSource = _qmlTabletRoot->property(LOADER_SOURCE_PROPERTY_NAME).toString();
        if (tabletSource != WEB_VIEW_SOURCE_URL) {
            QMetaObject::invokeMethod(_qmlTabletRoot, "loadSource", Q_ARG(const QVariant&, QVariant(WEB_VIEW_SOURCE_URL)));
            // TABLET_UI_HACK: TODO: addEventBridge to tablet....
        }
        QMetaObject::invokeMethod(_qmlTabletRoot, "loadWebUrl", Q_ARG(const QVariant&, QVariant(url)));
    }
}

QObject* TabletProxy::addButton(const QVariant& properties) {
    auto tabletButtonProxy = QSharedPointer<TabletButtonProxy>(new TabletButtonProxy(properties.toMap()));
    std::lock_guard<std::mutex> guard(_mutex);
    _tabletButtonProxies.push_back(tabletButtonProxy);
    if (_qmlTabletRoot) {
        auto tablet = getQmlTablet();
        if (tablet) {
            addButtonProxyToQmlTablet(tablet, tabletButtonProxy.data());
        } else {
            qCCritical(scriptengine) << "Could not find tablet in TabletRoot.qml";
        }
    }
    return tabletButtonProxy.data();
}

void TabletProxy::removeButton(QObject* tabletButtonProxy) {
    std::lock_guard<std::mutex> guard(_mutex);

    auto tablet = getQmlTablet();
    if (!tablet) {
        qCCritical(scriptengine) << "Could not find tablet in TabletRoot.qml";
    }

    auto iter = std::find(_tabletButtonProxies.begin(), _tabletButtonProxies.end(), tabletButtonProxy);
    if (iter != _tabletButtonProxies.end()) {
        if (_qmlTabletRoot) {
            (*iter)->setQmlButton(nullptr);
            if (tablet) {
                QMetaObject::invokeMethod(tablet, "removeButtonProxy", Qt::AutoConnection, Q_ARG(QVariant, (*iter)->getProperties()));
            }
        }
        _tabletButtonProxies.erase(iter);
    } else {
        qCWarning(scriptengine) << "TabletProxy::removeButton() could not find button " << tabletButtonProxy;
    }
}

void TabletProxy::emitScriptEvent(QVariant msg) {
    if (_qmlOffscreenSurface) {
        QMetaObject::invokeMethod(_qmlOffscreenSurface, "emitScriptEvent", Qt::AutoConnection, Q_ARG(QVariant, msg));
    }
}

void TabletProxy::addButtonsToHomeScreen() {
    auto tablet = getQmlTablet();
    if (!tablet) {
        return;
    }

    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    for (auto& buttonProxy : _tabletButtonProxies) {
        addButtonProxyToQmlTablet(tablet, buttonProxy.data());
    }
}

void TabletProxy::removeButtonsFromHomeScreen() {
    auto tabletScriptingInterface = DependencyManager::get<TabletScriptingInterface>();
    for (auto& buttonProxy : _tabletButtonProxies) {
        buttonProxy->setQmlButton(nullptr);
    }
}

QQuickItem* TabletProxy::getQmlTablet() const {
    if (!_qmlTabletRoot) {
        return nullptr;
    }

    auto loader = _qmlTabletRoot->findChild<QQuickItem*>("loader");
    if (!loader) {
        return nullptr;
    }

    auto tablet = loader->findChild<QQuickItem*>("tablet");
    if (!tablet) {
        return nullptr;
    }

    return tablet;
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

QVariantMap TabletButtonProxy::getProperties() const {
    std::lock_guard<std::mutex> guard(_mutex);
    return _properties;
}

void TabletButtonProxy::editProperties(QVariantMap properties) {
    std::lock_guard<std::mutex> guard(_mutex);
    QVariantMap::const_iterator iter = properties.constBegin();
    while (iter != properties.constEnd()) {
        _properties[iter.key()] = iter.value();
        if (_qmlButton) {
            QMetaObject::invokeMethod(_qmlButton, "changeProperty", Qt::AutoConnection, Q_ARG(QVariant, QVariant(iter.key())), Q_ARG(QVariant, iter.value()));
        }
        ++iter;
    }
}

#include "TabletScriptingInterface.moc"
