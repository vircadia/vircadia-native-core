//
//  SettingManager.h
//
//
//  Created by Clement on 2/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SettingManager_h
#define hifi_SettingManager_h

#include <QPointer>
#include <QSettings>
#include <QTimer>

#include "shared/ReadWriteLockable.h"

namespace Setting {
    class Interface;

    class Manager : public QSettings, public ReadWriteLockable {
        Q_OBJECT
    protected:
        ~Manager();
        void registerHandle(Interface* handle);
        void removeHandle(const QString& key);

        void loadSetting(Interface* handle);
        void saveSetting(Interface* handle);

    private slots:
        void startTimer();
        void stopTimer();

        void saveAll();

    private:
        QHash<QString, Interface*> _handles;
        QPointer<QTimer> _saveTimer = nullptr;

        friend class Interface;
        friend void cleanupPrivateInstance();
        friend void setupPrivateInstance();
    };
}

#endif // hifi_SettingManager_h
