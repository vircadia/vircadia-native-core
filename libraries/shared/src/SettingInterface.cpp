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

namespace Setting {
    // cleans up the settings private instance. Should only be run once at closing down.
    void cleanupPrivateInstance() {
        auto globalManager = DependencyManager::get<Manager>();
        Q_ASSERT(qApp && globalManager);

        // grab the thread before we nuke the instance
        QThread* settingsManagerThread = globalManager->thread();

        // quit the settings manager thread
        settingsManagerThread->quit();
        settingsManagerThread->wait();

        // Save all settings
        globalManager->saveAll();

        qCDebug(shared) << "Settings thread stopped.";
    }

    void setupPrivateInstance() {
        auto globalManager = DependencyManager::get<Manager>();
        Q_ASSERT(qApp && globalManager);

        // Let's set up the settings private instance on its own thread
        QThread* thread = new QThread(qApp);
        Q_CHECK_PTR(thread);
        thread->setObjectName("Settings Thread");

        // Setup setting periodical save timer
        QObject::connect(thread, &QThread::started, globalManager.data(), &Manager::startTimer);
        QObject::connect(thread, &QThread::finished, globalManager.data(), &Manager::stopTimer);

        // Setup manager threading affinity
        globalManager->moveToThread(thread);
        QObject::connect(thread, &QThread::finished, globalManager.data(), [] {
            auto globalManager = DependencyManager::get<Manager>();
            Q_ASSERT(qApp && globalManager);

            // Move manager back to the main thread (has to be done on owning thread)
            globalManager->moveToThread(qApp->thread());
        });
        
        thread->start();
        qCDebug(shared) << "Settings thread started.";

        // Register cleanupPrivateInstance to run inside QCoreApplication's destructor.
        qAddPostRoutine(cleanupPrivateInstance);
    }

    // Sets up the settings private instance. Should only be run once at startup. preInit() must be run beforehand,
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

        // Setup settings manager
        DependencyManager::set<Manager>();

        // Add pre-routine to setup threading
        qAddPreRoutine(setupPrivateInstance);
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
