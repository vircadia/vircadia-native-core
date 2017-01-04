//
//  SettingInterface.h
//
//
//  Created by Clement on 2/2/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SettingInterface_h
#define hifi_SettingInterface_h

#include <memory>
#include <QtCore/QWeakPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>

namespace Setting {
    class Manager;

    void init();
    void cleanupSettings();

    class Interface {
    public:
        const QString& getKey() const { return _key; }
        bool isSet() const { return _isSet; } 

        virtual void setVariant(const QVariant& variant) = 0;
        virtual QVariant getVariant() = 0;
        
    protected:
        Interface(const QString& key) : _key(key) {}
        virtual ~Interface() = default;

        void init();
        void maybeInit() const;
        void deinit();
        
        void save();
        void load();

        bool _isSet = false;
        const QString _key;

    private:
        mutable bool _isInitialized = false;
        
        friend class Manager;
        mutable QWeakPointer<Manager> _manager;
    };
}

#endif // hifi_SettingInterface_h
