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

template <typename T>
class SettingHandle {
public:
    SettingHandle(const QString& key, const T& defaultValue);
    
    T get() const; // Returns setting value, returns its default value if not found
    T get(const T& other) const; // Returns setting value, returns other if not found
    T getDefault() const;
    
    void set(const T& value) const;
    void reset() const;
    
    static const QVariant::Type type;
    
private:
    const QString _key;
    const QVariant _defaultValue;
};

class SettingsBridge {
private:
    static QVariant getFromSettings(const QString& key, const QVariant& defaultValue);
    static void setInSettings(const QString& key, const QVariant& value);
    
    template<typename T>
    friend class SettingHandle;
};

// Specialize template here for types used in Setting
template<typename T> const QVariant::Type SettingHandle<T>::type = QVariant::Invalid;
template<> const QVariant::Type SettingHandle<int>::type = QVariant::Int;
template<> const QVariant::Type SettingHandle<bool>::type = QVariant::Bool;
template<> const QVariant::Type SettingHandle<float>::type = QVariant::Double;
template<> const QVariant::Type SettingHandle<double>::type = QVariant::Double;
template<> const QVariant::Type SettingHandle<QString>::type = QVariant::String;

template <typename T>
SettingHandle<T>::SettingHandle(const QString& key, const T& defaultValue) : _key(key), _defaultValue(defaultValue) {
    // Will fire if template not specialized for that type below
    Q_STATIC_ASSERT_X(SettingHandle<T>::type != QVariant::Invalid, "SettingHandle: Invalid type");
}

template <typename T>
T SettingHandle<T>::get() const {
    QVariant variant = SettingsBridge::getFromSettings(_key, _defaultValue);
    if (variant.type() == type) {
        return variant.value<T>();
    }
    return _defaultValue.value<T>();
}

template <typename T>
T SettingHandle<T>::get(const T& other) const {
    QVariant variant = SettingsBridge::getFromSettings(_key, _defaultValue);
    if (variant.type() == type) {
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
    SettingsBridge::setInSettings(_key, QVariant(value));
}

template <typename T> inline
void SettingHandle<T>::reset() const {
    setInSettings(_key, _defaultValue);
}

// Put applicationwide settings here
namespace SettingHandles {

}

#endif // hifi_Settings_h