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
    
    // Sets up the settings private instance. Should only be run once at startup
    void setupPrivateInstance() {
        // read the ApplicationInfo.ini file for Name/Version/Domain information
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings applicationInfo(PathUtils::resourcesPath() + "info/ApplicationInfo.ini", QSettings::IniFormat);
        // set the associated application properties
        applicationInfo.beginGroup("INFO");
        QCoreApplication::setApplicationName(applicationInfo.value("name").toString());
        QCoreApplication::setOrganizationName(applicationInfo.value("organizationName").toString());
        QCoreApplication::setOrganizationDomain(applicationInfo.value("organizationDomain").toString());
        
        // Let's set up the settings Private instance on it's own thread
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
    // Register setupPrivateInstance to run after QCoreApplication's constructor.
    Q_COREAPP_STARTUP_FUNCTION(setupPrivateInstance)
    
    
    Interface::~Interface() {
        if (privateInstance) {
            privateInstance->removeHandle(_key);
        }
    }
    
    void Interface::init() {
        if (!privateInstance) {
            qWarning() << "Setting::Interface::init(): Manager not yet created, bailing";
            return;
        }
        
        // Register Handle
        privateInstance->registerHandle(this);
        _isInitialized = true;
        
        // Load value from disk
        load();
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
