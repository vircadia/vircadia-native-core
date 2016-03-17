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

#include <QThread>
#include <QDebug>

#include "SettingInterface.h"
#include "SettingManager.h"

namespace Setting {
    Manager::~Manager() {
        // Cleanup timer
        stopTimer();
        delete _saveTimer;

        // Save all settings before exit
        saveAll();

        // sync will be called in the QSettings destructor
    }

    void Manager::registerHandle(Setting::Interface* handle) {
        QString key = handle->getKey();
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
        handle->setVariant(value(handle->getKey()));
    }

    void Manager::saveSetting(Interface* handle) {
        if (handle->isSet()) {
            setValue(handle->getKey(), handle->getVariant());
        } else {
            remove(handle->getKey());
        }
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
        withReadLock([&] {
            for (auto handle : _handles) {
                saveSetting(handle);
            }
        });

        // Restart timer
        if (_saveTimer) {
            _saveTimer->start();
        }
    }
}
