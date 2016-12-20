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

        // Save all settings before exit
        saveAll();

        // sync will be called in the QSettings destructor
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
                loadedValue = value(key);
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
            _saveTimer->setInterval(SAVE_INTERVAL_MSEC);
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
        bool forceSync = false;
        withWriteLock([&] {
            for (auto key : _pendingChanges.keys()) {
                auto newValue = _pendingChanges[key];
                auto savedValue = value(key, UNSET_VALUE);
                if (newValue == savedValue) {
                    continue;
                }
                if (newValue == UNSET_VALUE || !newValue.isValid()) {
                    forceSync = true;
                    remove(key);
                } else {
                    forceSync = true;
                    setValue(key, newValue);
                }
            }
            _pendingChanges.clear();
        });

        if (forceSync) {
            sync();
        }

        // Restart timer
        if (_saveTimer) {
            _saveTimer->start();
        }
    }
}
