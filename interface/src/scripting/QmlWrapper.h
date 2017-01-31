//
//  Created by Anthony J. Thibault on 2016-12-12
//  Copyright 2013-2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_QmlWrapper_h
#define hifi_QmlWrapper_h

#include <QtCore/QObject>
#include <OffscreenUi.h>
#include <DependencyManager.h>

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

#endif