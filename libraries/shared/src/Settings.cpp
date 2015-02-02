//
//  Settings.cpp
//
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QCoreApplication>
#include <QDebug>
#include <QHash>
#include <QSettings>
#include <QThread>
#include <QVector>

#include "PathUtils.h"
#include "Settings.h"

namespace Setting {
    class Manager : public QSettings {
    public:
        ~Manager();
        void registerHandle(Interface* handle);
        void removeHandle(const QString& key);
        
        void loadSetting(Interface* handle);
        void saveSetting(Interface* handle);
        void saveAll();
        
    private:
        QHash<QString, Interface*> _handles;
    };
    Manager* privateInstance = nullptr;
    
    // cleans up the settings private instance. Should only be run once at closing down.
    void cleanupPrivateInstance() {
        delete privateInstance;
        privateInstance = nullptr;
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
        privateInstance = new Manager();
        Q_CHECK_PTR(privateInstance);
        thread->setObjectName("Settings Thread");
        QObject::connect(privateInstance, SIGNAL(destroyed()), thread, SLOT(quit()));
        QObject::connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
        privateInstance->moveToThread(thread);
        thread->start();
        qDebug() << "Settings thread started.";
        
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
        if (privateInstance) {
            // Register Handle
            privateInstance->registerHandle(this);
            _isInitialized = true;
            
            // Load value from disk
            privateInstance->loadSetting(this);
        } else {
            qWarning() << "Setting::Interface::init(): Manager not yet created";
        }
    }
    
    void Interface::maybeInit() {
        if (!_isInitialized) {
            init();
        }
    }
    
    Manager::~Manager() {
        saveAll();
        sync();
    }
    
    void Manager::registerHandle(Setting::Interface* handle) {
        QString key = handle->getKey();
        if (_handles.contains(key)) {
            qWarning() << "Setting::Manager::registerHandle(): Key registered more than once, overriding: " << key;
        }
        _handles.insert(key, handle);
    }
    
    void Manager::removeHandle(const QString& key) {
        _handles.remove(key);
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
    
    void Manager::saveAll() {
        for (auto handle : _handles) {
            saveSetting(handle);
        }
    }
    
}