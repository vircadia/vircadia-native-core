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
#include "QmlWrapper.h"

class ToolbarButtonProxy : public QmlWrapper {
    Q_OBJECT

public:
    ToolbarButtonProxy(QObject* qmlObject, QObject* parent = nullptr) : QmlWrapper(qmlObject, parent) {
        std::lock_guard<std::mutex> guard(_mutex);
        _qmlButton = qobject_cast<QQuickItem*>(qmlObject);
        connect(qmlObject, SIGNAL(clicked()), this, SIGNAL(clicked()));
    }

    Q_INVOKABLE void editProperties(QVariantMap properties) {
        std::lock_guard<std::mutex> guard(_mutex);
        QVariantMap::const_iterator iter = properties.constBegin();
        while (iter != properties.constEnd()) {
            _properties[iter.key()] = iter.value();
            if (_qmlButton) {
                // [01/25 14:26:20] [WARNING] [default] QMetaObject::invokeMethod: No such method ToolbarButton_QMLTYPE_195::changeProperty(QVariant,QVariant)

                QMetaObject::invokeMethod(_qmlButton, "changeProperty", Qt::AutoConnection,
                                          Q_ARG(QVariant, QVariant(iter.key())), Q_ARG(QVariant, iter.value()));
            }
            ++iter;
        }
    }

signals:
    void clicked();

protected:
    mutable std::mutex _mutex;
    QQuickItem* _qmlButton { nullptr };
    QVariantMap _properties;
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
