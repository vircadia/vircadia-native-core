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

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QThread>

#include "PathUtils.h"
#include "SettingInterface.h"
#include "SettingManager.h"
#include "SharedLogging.h"

namespace Setting {
    static Manager* privateInstance = nullptr;
    
    // cleans up the settings private instance. Should only be run once at closing down.
    void cleanupPrivateInstance() {
        // grab the thread before we nuke the instance
        QThread* settingsManagerThread = privateInstance->thread();
        
        // tell the private instance to clean itself up on its thread
        privateInstance->deleteLater();
        privateInstance = NULL;
        
        // quit the settings manager thread and wait on it to make sure it's gone
        settingsManagerThread->quit();
        settingsManagerThread->wait();
    }

    // Set up application settings. Should only be run once at startup.
    void preInit() {
        // read the ApplicationInfo.ini file for Name/Version/Domain information
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings applicationInfo(PathUtils::resourcesPath() + "info/ApplicationInfo.ini", QSettings::IniFormat);
        // set the associated application properties
        applicationInfo.beginGroup("INFO");
        QCoreApplication::setApplicationName(applicationInfo.value("name").toString());
        QCoreApplication::setOrganizationName(applicationInfo.value("organizationName").toString());
        QCoreApplication::setOrganizationDomain(applicationInfo.value("organizationDomain").toString());

        // Delete Interface.ini.lock file if it exists, otherwise Interface freezes.
        QSettings settings;
        QString settingsLockFilename = settings.fileName() + ".lock";
        QFile settingsLockFile(settingsLockFilename);
        if (settingsLockFile.exists()) {
            bool deleted = settingsLockFile.remove();
            qCDebug(shared) << (deleted ? "Deleted" : "Failed to delete") << "settings lock file" << settingsLockFilename;
        }
    }
    
    // Sets up the settings private instance. Should only be run once at startup. preInit() must be run beforehand,
    void init() {
        // Let's set up the settings Private instance on its own thread
        QThread* thread = new QThread();
        Q_CHECK_PTR(thread);
        thread->setObjectName("Settings Thread");
        
        privateInstance = new Manager();
        Q_CHECK_PTR(privateInstance);

        QObject::connect(privateInstance, SIGNAL(destroyed()), thread, SLOT(quit()));
        QObject::connect(thread, SIGNAL(started()), privateInstance, SLOT(startTimer()));
        QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        privateInstance->moveToThread(thread);
        thread->start();
        qCDebug(shared) << "Settings thread started.";    

        // Register cleanupPrivateInstance to run inside QCoreApplication's destructor.
        qAddPostRoutine(cleanupPrivateInstance);
    }    
    
    Interface::~Interface() {
        if (privateInstance) {
            privateInstance->removeHandle(_key);
        }
    }
    
    void Interface::init() {
        if (!privateInstance) {
            // WARNING: As long as we are using QSettings this should always be triggered for each Setting::Handle
            // in an assignment-client - the QSettings backing we use for this means persistence of these
            // settings from an AC (when there can be multiple terminating at same time on one machine)
            // is currently not supported
            qWarning() << "Setting::Interface::init() for key" << _key << "- Manager not yet created." << 
                "Settings persistence disabled.";
        } else {
            // Register Handle
            privateInstance->registerHandle(this);
            _isInitialized = true;
        
            // Load value from disk
            load();
        }
    }
    
    void Interface::maybeInit() {
        if (!_isInitialized) {
            init();
        }
    }
    
    void Interface::save() {
        if (privateInstance) {
            privateInstance->saveSetting(this);
        }
    }
    
    void Interface::load() {
        if (privateInstance) {
            privateInstance->loadSetting(this);
        }
    }
}
