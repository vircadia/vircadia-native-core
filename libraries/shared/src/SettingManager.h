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

#include <QtCore/QPointer>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtCore/QUuid>

#include "DependencyManager.h"
#include "shared/ReadWriteLockable.h"

namespace Setting {
    class Interface;

    class Manager : public QObject, public ReadWriteLockable, public Dependency {
        Q_OBJECT

    public:
        void customDeleter() override;

        // thread-safe proxies into QSettings
        QString fileName() const;
        void remove(const QString &key);
        QStringList childGroups() const;
        QStringList childKeys() const;
        QStringList allKeys() const;
        bool contains(const QString &key) const;
        int beginReadArray(const QString &prefix);
        void beginGroup(const QString &prefix);
        void beginWriteArray(const QString &prefix, int size = -1);
        void endArray();
        void endGroup();
        void setArrayIndex(int i);
        void setValue(const QString &key, const QVariant &value);
        QVariant value(const QString &key, const QVariant &defaultValue = QVariant()) const;

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
        const QVariant UNSET_VALUE { QUuid::createUuid() };
        QHash<QString, QVariant> _pendingChanges;

        friend class Interface;
        friend void cleanupSettingsSaveThread();
        friend void setupSettingsSaveThread();


        QSettings _qSettings;
    };
}

#endif // hifi_SettingManager_h
