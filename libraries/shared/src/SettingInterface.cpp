//
//  SettingInterface.cpp
//  libraries/shared/src
//
//  Created by Clement on 2/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SettingInterface.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QThread>

#include "PathUtils.h"
#include "SettingHelpers.h"
#include "SettingManager.h"
#include "SharedLogging.h"
#include "SharedUtil.h"
#include "ThreadHelpers.h"

namespace Setting {
    // This should only run as a post-routine in the QCoreApplication destructor
    void cleanupSettingsSaveThread() {
        auto globalManager = DependencyManager::get<Manager>();
        Q_ASSERT(qApp && globalManager);

        // Grab the settings thread to shut it down
        QThread* settingsManagerThread = globalManager->thread();

        // Quit the settings manager thread and wait for it so we
        // don't get concurrent accesses when we save all settings below
        settingsManagerThread->quit();
        settingsManagerThread->wait();

        // [IMPORTANT] Save all settings when the QApplication goes down
        globalManager->saveAll();

        qCDebug(shared) << "Settings thread stopped.";
    }

    // This should only run as a pre-routine in the QCoreApplication constructor
    void setupSettingsSaveThread() {
        auto globalManager = DependencyManager::get<Manager>();
        Q_ASSERT(qApp && globalManager);

        // Let's set up the settings private instance on its own thread
        QThread* thread = new QThread(qApp);
        Q_CHECK_PTR(thread);
        thread->setObjectName("Settings Thread");

        // Setup setting periodical save timer
        QObject::connect(thread, &QThread::started, globalManager.data(), [globalManager] {
            setThreadName("Settings Save Thread");
            globalManager->startTimer();
        });
        QObject::connect(thread, &QThread::finished, globalManager.data(), &Manager::stopTimer);

        // Setup manager threading affinity
        // This makes the timer fire on the settings thread so we don't block the main
        // thread with a lot of file I/O.
        // We bring back the manager to the main thread when the QApplication goes down
        globalManager->moveToThread(thread);
        QObject::connect(thread, &QThread::finished, globalManager.data(), [] {
            auto globalManager = DependencyManager::get<Manager>();
            Q_ASSERT(qApp && globalManager);

            // Move manager back to the main thread (has to be done on owning thread)
            globalManager->moveToThread(qApp->thread());
        });

        // Start the settings save thread
        thread->start();
        qCDebug(shared) << "Settings thread started.";

        // Register cleanupSettingsSaveThread to run inside QCoreApplication's destructor.
        // This will cleanup the settings thread and save all settings before shut down.
        qAddPostRoutine(cleanupSettingsSaveThread);
    }

    // Sets up the settings private instance. Should only be run once at startup.
    void init() {
        // Set settings format
        QSettings::setDefaultFormat(JSON_FORMAT);
        QSettings settings;
        qCDebug(shared) << "Settings file:" << settings.fileName();

        // Backward compatibility for old settings file
        if (settings.allKeys().isEmpty()) {
            loadOldINIFile(settings);
        }

        // Delete Interface.ini.lock file if it exists, otherwise Interface freezes.
        QString settingsLockFilename = settings.fileName() + ".lock";
        QFile settingsLockFile(settingsLockFilename);
        if (settingsLockFile.exists()) {
            bool deleted = settingsLockFile.remove();
            qCDebug(shared) << (deleted ? "Deleted" : "Failed to delete") << "settings lock file" << settingsLockFilename;
        }

        // Setup settings manager, the manager will live until the process shuts down
        DependencyManager::set<Manager>();

        // Add pre-routine to setup threading
        qAddPreRoutine(setupSettingsSaveThread);
    }

    void Interface::init() {
        if (!DependencyManager::isSet<Manager>()) {
            // WARNING: As long as we are using QSettings this should always be triggered for each Setting::Handle
            // in an assignment-client - the QSettings backing we use for this means persistence of these
            // settings from an AC (when there can be multiple terminating at same time on one machine)
            // is currently not supported
            qWarning() << "Setting::Interface::init() for key" << _key << "- Manager not yet created." << 
                "Settings persistence disabled.";
        } else {
            _manager = DependencyManager::get<Manager>();
            auto manager = _manager.lock();
            if (manager) {
                // Register Handle
                manager->registerHandle(this);
                _isInitialized = true;
            } else {
                qWarning() << "Settings interface used after manager destroyed";
            }
        
            // Load value from disk
            load();
        }
    }

    void Interface::deinit() {
        if (_isInitialized && _manager) {
            auto manager = _manager.lock();
            if (manager) {
                // Save value to disk
                save();
                manager->removeHandle(_key);
            }
        }
    }

    
    void Interface::maybeInit() const {
        if (!_isInitialized) {
            const_cast<Interface*>(this)->init();
        }
    }
    
    void Interface::save() {
        auto manager = _manager.lock();
        if (manager) {
            manager->saveSetting(this);
        }
    }
    
    void Interface::load() {
        auto manager = _manager.lock();
        if (manager) {
            manager->loadSetting(this);
        }
    }
}
