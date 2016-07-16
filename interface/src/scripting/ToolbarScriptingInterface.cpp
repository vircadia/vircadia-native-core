//
//  Created by Bradley Austin Davis on 2016-06-16
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "ToolbarScriptingInterface.h"

#include <QtCore/QThread>

#include <OffscreenUi.h>

class QmlWrapper : public QObject {
    Q_OBJECT
public:
    QmlWrapper(QObject* qmlObject, QObject* parent = nullptr)
        : QObject(parent), _qmlObject(qmlObject) {
    }

    Q_INVOKABLE void writeProperty(QString propertyName, QVariant propertyValue) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->executeOnUiThread([=] {
            _qmlObject->setProperty(propertyName.toStdString().c_str(), propertyValue);
        });
    }

    Q_INVOKABLE void writeProperties(QVariant propertyMap) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        offscreenUi->executeOnUiThread([=] {
            QVariantMap map = propertyMap.toMap();
            for (const QString& key : map.keys()) {
                _qmlObject->setProperty(key.toStdString().c_str(), map[key]);
            }
        });
    }

    Q_INVOKABLE QVariant readProperty(const QString& propertyName) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        return offscreenUi->returnFromUiThread([&]()->QVariant {
            return _qmlObject->property(propertyName.toStdString().c_str());
        });
    }

    Q_INVOKABLE QVariant readProperties(const QVariant& propertyList) {
        auto offscreenUi = DependencyManager::get<OffscreenUi>();
        return offscreenUi->returnFromUiThread([&]()->QVariant {
            QVariantMap result;
            for (const QVariant& property : propertyList.toList()) {
                QString propertyString = property.toString();
                result.insert(propertyString, _qmlObject->property(propertyString.toStdString().c_str()));
            }
            return result;
        });
    }


protected:
    QObject* _qmlObject{ nullptr };
};


class ToolbarButtonProxy : public QmlWrapper {
    Q_OBJECT

public:
    ToolbarButtonProxy(QObject* qmlObject, QObject* parent = nullptr) : QmlWrapper(qmlObject, parent) {
        connect(qmlObject, SIGNAL(clicked()), this, SIGNAL(clicked()));
    }

signals:
    void clicked();
};

class ToolbarProxy : public QmlWrapper {
    Q_OBJECT

public:
    ToolbarProxy(QObject* qmlObject, QObject* parent = nullptr) : QmlWrapper(qmlObject, parent) { }

    Q_INVOKABLE QObject* addButton(const QVariant& properties) {
        QVariant resultVar;
        Qt::ConnectionType connectionType = Qt::AutoConnection;
        if (QThread::currentThread() != _qmlObject->thread()) {
            connectionType = Qt::BlockingQueuedConnection;
        }
        bool invokeResult = QMetaObject::invokeMethod(_qmlObject, "addButton", connectionType, Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, properties));
        if (!invokeResult) {
            return nullptr;
        }

        QObject* rawButton = qvariant_cast<QObject *>(resultVar);
        if (!rawButton) {
            return nullptr;
        }

        return new ToolbarButtonProxy(rawButton, this);
    }

    Q_INVOKABLE void removeButton(const QVariant& name) {
        QMetaObject::invokeMethod(_qmlObject, "removeButton", Qt::AutoConnection, Q_ARG(QVariant, name));
    }
};


QObject* ToolbarScriptingInterface::getToolbar(const QString& toolbarId) {
    auto offscreenUi = DependencyManager::get<OffscreenUi>();
    auto desktop = offscreenUi->getDesktop();
    Qt::ConnectionType connectionType = Qt::AutoConnection;
    if (QThread::currentThread() != desktop->thread()) {
        connectionType = Qt::BlockingQueuedConnection;
    }
    QVariant resultVar;
    bool invokeResult = QMetaObject::invokeMethod(desktop, "getToolbar", connectionType, Q_RETURN_ARG(QVariant, resultVar), Q_ARG(QVariant, toolbarId));
    if (!invokeResult) {
        return nullptr;
    }

    QObject* rawToolbar = qvariant_cast<QObject *>(resultVar);
    if (!rawToolbar) {
        return nullptr;
    }

    return new ToolbarProxy(rawToolbar);
}


#include "ToolbarScriptingInterface.moc"
