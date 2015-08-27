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

#include <QString>
#include <QVariant>

namespace Setting {
    void preInit();
    void init();
    void cleanupSettings();

    class Interface {
    public:
        QString getKey() const { return _key; }
        bool isSet() const { return _isSet; } 

        virtual void setVariant(const QVariant& variant) = 0;
        virtual QVariant getVariant() = 0;
        
    protected:
        Interface(const QString& key) : _key(key) {}
        virtual ~Interface();

        void init();
        void maybeInit();
        
        void save();
        void load();
        
        bool _isInitialized = false;
        bool _isSet = false;
        const QString _key;
        
        friend class Manager;
    };
}

#endif // hifi_SettingInterface_h
