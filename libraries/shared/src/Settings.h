//
//  Settings.h
//
//
//  Created by Clement on 1/18/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Settings_h
#define hifi_Settings_h

#include <QSettings>
#include <QString>
#include <QVariant>

// TODO: remove
class Settings : public QSettings {
    
};

namespace SettingHandles {
    
template <typename T>
class SettingHandle {
public:
    SettingHandle(const QString& key);
    SettingHandle(const QStringList& path);
    SettingHandle(const QString& key, const T& defaultValue);
    SettingHandle(const QStringList& path, const T& defaultValue);
    
    T get() const; // Returns setting value, returns its default value if not found
    T get(const T& other) const; // Returns setting value, returns other if not found
    T getDefault() const;
    
    void set(const T& value) const;
    void reset() const;
    
    void remove() const;
    
private:
    const QString _key;
    const QVariant _defaultValue;
};

class SettingsBridge {
private:
    static QVariant getFromSettings(const QString& key, const QVariant& defaultValue);
    static void setInSettings(const QString& key, const QVariant& value);
    static void removeFromSettings(const QString& key);
    
    template<typename T>
    friend class SettingHandle;
};
    
template <typename T>
SettingHandle<T>::SettingHandle(const QString& key) : _key(key) {
}

template <typename T>
SettingHandle<T>::SettingHandle(const QStringList& path) : _key(path.join("/")) {
}
    
template <typename T>
SettingHandle<T>::SettingHandle(const QString& key, const T& defaultValue) :
    _key(key),
    _defaultValue(defaultValue) {
}

template <typename T>
SettingHandle<T>::SettingHandle(const QStringList& path, const T& defaultValue) :
    _key(path.join("/")),
    _defaultValue(defaultValue) {
        
}

template <typename T>
T SettingHandle<T>::get() const {
    QVariant variant = SettingsBridge::getFromSettings(_key, _defaultValue);
    if (variant.canConvert<T>()) {
        return variant.value<T>();
    }
    return _defaultValue.value<T>();
}

template <typename T>
T SettingHandle<T>::get(const T& other) const {
    QVariant variant = SettingsBridge::getFromSettings(_key, QVariant(other));
    if (variant.canConvert<T>()) {
        return variant.value<T>();
    }
    return other;
}

template <typename T> inline
T SettingHandle<T>::getDefault() const {
    return _defaultValue.value<T>();
}

template <typename T> inline
void SettingHandle<T>::set(const T& value) const {
    if (value != get()) {
        SettingsBridge::setInSettings(_key, QVariant(value));
    }
}

template <typename T> inline
void SettingHandle<T>::reset() const {
    SettingsBridge::setInSettings(_key, _defaultValue);
}
    
template <typename T> inline
void SettingHandle<T>::remove() const {
    SettingsBridge::removeFromSettings(_key);
}
    
}

#endif // hifi_Settings_h