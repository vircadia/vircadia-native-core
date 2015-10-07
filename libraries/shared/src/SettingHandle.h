//
//  SettingHandle.h
//
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SettingHandle_h
#define hifi_SettingHandle_h

#include <type_traits>

#include <QSettings>
#include <QString>
#include <QVariant>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "SettingInterface.h"

// TODO: remove
class Settings : public QSettings {
public:
    void getFloatValueIfValid(const QString& name, float& floatValue);
    void getBoolValue(const QString& name, bool& boolValue);

    void setVec3Value(const QString& name, const glm::vec3& vecValue);
    void getVec3ValueIfValid(const QString& name, glm::vec3& vecValue);

    void setQuatValue(const QString& name, const glm::quat& quatValue);
    void getQuatValueIfValid(const QString& name, glm::quat& quatValue);
};

namespace Setting {
    template <typename T>
    class Handle : public Interface {
    public:
        Handle(const QString& key) : Interface(key) {}
        Handle(const QStringList& path) : Interface(path.join("/")) {}
        
        Handle(const QString& key, const T& defaultValue) : Interface(key), _defaultValue(defaultValue) {}
        Handle(const QStringList& path, const T& defaultValue) : Handle(path.join("/"), defaultValue) {}
        
        virtual ~Handle() { save(); }
        
        // Returns setting value, returns its default value if not found
        T get() { return get(_defaultValue); }
        // Returns setting value, returns other if not found
        T get(const T& other) { maybeInit(); return (_isSet) ? _value : other; }
        T getDefault() const { return _defaultValue; }
        
        void set(const T& value) { maybeInit(); _value = value; _isSet = true; }
        void reset() { set(_defaultValue); }
        
        void remove() { maybeInit(); _isSet = false; }
        
    protected:
        virtual void setVariant(const QVariant& variant);
        virtual QVariant getVariant() { return QVariant::fromValue(get()); }
        
    private:
        T _value;
        const T _defaultValue;
    };
    
    template <typename T>
    void Handle<T>::setVariant(const QVariant& variant) {
        if (variant.canConvert<T>() || std::is_same<T, QVariant>::value) {
            set(variant.value<T>());
        }
    }
}

#endif // hifi_SettingHandle_h
