//
//  SettingManager.cpp
//
//
//  Created by Clement on 2/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SettingManager.h"

#include <QtCore/QThread>
#include <QtCore/QDebug>
#include <QtCore/QUuid>

#include "SettingInterface.h"

namespace Setting {

    Manager::~Manager() {
        // Cleanup timer
        stopTimer();
        delete _saveTimer;
        _saveTimer = nullptr;
    }

    // Custom deleter does nothing, because we need to shutdown later than the dependency manager
    void Manager::customDeleter() { }

    void Manager::registerHandle(Interface* handle) {
        const QString& key = handle->getKey();
        withWriteLock([&] {
            if (_handles.contains(key)) {
                qWarning() << "Setting::Manager::registerHandle(): Key registered more than once, overriding: " << key;
            }
            _handles.insert(key, handle);
        });
    }

    void Manager::removeHandle(const QString& key) {
        withWriteLock([&] {
            _handles.remove(key);
        });
    }

    void Manager::loadSetting(Interface* handle) {
        const auto& key = handle->getKey();
        withWriteLock([&] {
            QVariant loadedValue;
            if (_pendingChanges.contains(key) && _pendingChanges[key] != UNSET_VALUE) {
                loadedValue = _pendingChanges[key];
            } else {
                loadedValue = _qSettings.value(key);
            }
            if (loadedValue.isValid()) {
                handle->setVariant(loadedValue);
            }
        });
    }


    void Manager::saveSetting(Interface* handle) {
        const auto& key = handle->getKey();
        QVariant handleValue = UNSET_VALUE;
        if (handle->isSet()) {
            handleValue = handle->getVariant();
        }

        withWriteLock([&] {
            _pendingChanges[key] = handleValue;
        });
    }

    static const int SAVE_INTERVAL_MSEC = 5 * 1000; // 5 sec
    void Manager::startTimer() {
        if (!_saveTimer) {
            _saveTimer = new QTimer(this);
            Q_CHECK_PTR(_saveTimer);
            _saveTimer->setSingleShot(true); // We will restart it once settings are saved.
            _saveTimer->setInterval(SAVE_INTERVAL_MSEC); // 5s, Qt::CoarseTimer acceptable
            connect(_saveTimer, SIGNAL(timeout()), this, SLOT(saveAll()));
        }
        _saveTimer->start();
    }

    void Manager::stopTimer() {
        if (_saveTimer) {
            _saveTimer->stop();
        }
    }

    void Manager::saveAll() {
        withWriteLock([&] {
            bool forceSync = false;
            for (auto key : _pendingChanges.keys()) {
                auto newValue = _pendingChanges[key];
                auto savedValue = _qSettings.value(key, UNSET_VALUE);
                if (newValue == savedValue) {
                    continue;
                }
                forceSync = true;
                if (newValue == UNSET_VALUE || !newValue.isValid()) {
                    _qSettings.remove(key);
                } else {
                    _qSettings.setValue(key, newValue);
                }
            }
            _pendingChanges.clear();

            if (forceSync) {
                _qSettings.sync();
            }
        });

        // Restart timer
        if (_saveTimer) {
            _saveTimer->start();
        }
    }

    QString Manager::fileName() const {
        return resultWithReadLock<QString>([&] {
            return _qSettings.fileName();
        });
    }

    void Manager::remove(const QString &key) {
        withWriteLock([&] {
            _qSettings.remove(key);
        });
    }

    QStringList Manager::childGroups() const {
        return resultWithReadLock<QStringList>([&] {
            return _qSettings.childGroups();
        });
    }

    QStringList Manager::childKeys() const {
        return resultWithReadLock<QStringList>([&] {
            return _qSettings.childKeys();
        });
    }

    QStringList Manager::allKeys() const {
        return resultWithReadLock<QStringList>([&] {
            return _qSettings.allKeys();
        });
    }

    bool Manager::contains(const QString &key) const {
        return resultWithReadLock<bool>([&] {
            return _qSettings.contains(key);
        });
    }

    int Manager::beginReadArray(const QString &prefix) {
        return resultWithReadLock<int>([&] {
            return _qSettings.beginReadArray(prefix);
        });
    }

    void Manager::beginGroup(const QString &prefix) {
        withWriteLock([&] {
            _qSettings.beginGroup(prefix);
        });
    }

    void Manager::beginWriteArray(const QString &prefix, int size) {
        withWriteLock([&] {
            _qSettings.beginWriteArray(prefix, size);
        });
    }

    void Manager::endArray() {
        withWriteLock([&] {
            _qSettings.endArray();
        });
    }

    void Manager::endGroup() {
        withWriteLock([&] {
            _qSettings.endGroup();
        });
    }

    void Manager::setArrayIndex(int i) {
        withWriteLock([&] {
            _qSettings.setArrayIndex(i);
        });
    }

    void Manager::setValue(const QString &key, const QVariant &value) {
        withWriteLock([&] {
            _qSettings.setValue(key, value);
        });
    }

    QVariant Manager::value(const QString &key, const QVariant &defaultValue) const {
        return resultWithReadLock<QVariant>([&] {
            return _qSettings.value(key, defaultValue);
        });
    }
}
