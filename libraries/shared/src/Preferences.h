//
//  Created by Bradley Austin Davis 2016/01/20
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_Shared_Preferences_h
#define hifi_Shared_Preferences_h

#include <functional>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QList>
#include <QtCore/QString>

#include "DependencyManager.h"

class Preference;

class Preferences : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(QVariantMap preferencesByCategory READ getPreferencesByCategory CONSTANT)
    Q_PROPERTY(QList<QString> categories READ getCategories CONSTANT)

public:
    void addPreference(Preference* preference);
    const QVariantMap& getPreferencesByCategory() { return _preferencesByCategory; }
    const QList<QString>& getCategories() { return _categories; }

private:
    QVariantMap _preferencesByCategory;
    QList<QString> _categories;
};

class BoolPreference;

class Preference : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString category READ getCategory CONSTANT)
    Q_PROPERTY(QString name READ getName CONSTANT)
    Q_PROPERTY(Type type READ getType CONSTANT)
    Q_PROPERTY(bool enabled READ isEnabled NOTIFY enabledChanged)
    Q_ENUMS(Type)

public:
    enum Type {
        Invalid,
        Editable,
        Browsable,
        Slider,
        Spinner,
        Checkbox,
        Button,
        ComboBox,
        // Special casing for an unusual preference
        Avatar
    };

    explicit Preference(QObject* parent = nullptr) : QObject(parent) {}
    Preference(const QString& category, const QString& name, QObject* parent = nullptr)
        : QObject(parent), _category(category), _name(name) { }

    const QString& getCategory() const { return _category; }
    const QString& getName() const { return _name; }
    bool isEnabled() const {
        return _enabled;
    }

    void setEnabled(bool enabled) {
        if (enabled != _enabled) {
            _enabled = enabled;
            emit enabledChanged();
        }
    }

    void setEnabler(BoolPreference* enabler, bool inverse = false);

    virtual Type getType() { return Invalid; };

    Q_INVOKABLE virtual void load() {};
    Q_INVOKABLE virtual void save() const {}

signals:
    void enabledChanged();

private slots:
    void onEnablerValueChanged();

protected:
    virtual void emitValueChanged() {};

    BoolPreference* _enabler { nullptr };
    const QString _category;
    const QString _name;
    bool _enabled { true };
    bool _enablerInverted { false };
};

class ButtonPreference : public Preference {
    Q_OBJECT
public:
    using Lambda = std::function<void()>;
    ButtonPreference(const QString& category, const QString& name, Lambda triggerHandler)
        : Preference(category, name), _triggerHandler(triggerHandler) { }
    Type getType() override { return Button; }
    Q_INVOKABLE void trigger() { _triggerHandler(); }

protected:
    const Lambda _triggerHandler;
};


template <typename T>
class TypedPreference : public Preference {
public:
    using Getter = std::function<T()>;
    using Setter = std::function<void(const T&)>;

    TypedPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : Preference(category, name), _getter(getter), _setter(setter) { }

    T getValue() const { return _value; }
    void setValue(const T& value) { if (_value != value) { _value = value; emitValueChanged(); } }
    void load() override { _value = _getter(); }
    void save() const override { 
        T oldValue = _getter();
        if (_value != oldValue) {
            _setter(_value); 
        } 
    }

protected:
    T _value;
    const Getter _getter;
    const Setter _setter;
};

class BoolPreference : public TypedPreference<bool> {
    Q_OBJECT
    Q_PROPERTY(bool value READ getValue WRITE setValue NOTIFY valueChanged)

public:
    BoolPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : TypedPreference(category, name, getter, setter) { }

signals:
    void valueChanged();

protected:
    void emitValueChanged() override { emit valueChanged(); }
};

class FloatPreference : public TypedPreference<float> {
    Q_OBJECT
    Q_PROPERTY(float value READ getValue WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(float min READ getMin CONSTANT)
    Q_PROPERTY(float max READ getMax CONSTANT)
    Q_PROPERTY(float step READ getStep CONSTANT)
    Q_PROPERTY(float decimals READ getDecimals CONSTANT)

public:
    FloatPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : TypedPreference(category, name, getter, setter) { }

    float getMin() const { return _min; }
    void setMin(float min) { _min = min; };

    float getMax() const { return _max; }
    void setMax(float max) { _max = max; };

    float getStep() const { return _step; }
    void setStep(float step) { _step = step; };

    float getDecimals() const { return _decimals; }
    void setDecimals(float decimals) { _decimals = decimals; };

signals:
    void valueChanged();

protected:
    void emitValueChanged() override { emit valueChanged(); }

    float _decimals { 0 };
    float _min { 0 };
    float _max { 1 };
    float _step { 0.1f };
};

class StringPreference : public TypedPreference<QString> {
    Q_OBJECT
    Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)

public:
    StringPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : TypedPreference(category, name, getter, setter) { }

signals:
    void valueChanged();

protected:
    void emitValueChanged() override { emit valueChanged(); }
};

class SliderPreference : public FloatPreference {
    Q_OBJECT
public:
    SliderPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : FloatPreference(category, name, getter, setter) { }

    Type getType() override { return Slider; }
};

class SpinnerPreference : public FloatPreference {
    Q_OBJECT
public:
    SpinnerPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : FloatPreference(category, name, getter, setter) { }

    Type getType() override { return Spinner; }
};

class EditPreference : public StringPreference {
    Q_OBJECT
    Q_PROPERTY(QString placeholderText READ getPlaceholderText CONSTANT)

public:
    EditPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : StringPreference(category, name, getter, setter) { }
    Type getType() override { return Editable; }
    const QString& getPlaceholderText() const { return _placeholderText; }
    void setPlaceholderText(const QString& placeholderText) { _placeholderText = placeholderText; }

protected:
    QString _placeholderText;
};

class ComboBoxPreference : public EditPreference {
    Q_OBJECT
    Q_PROPERTY(QStringList items READ getItems CONSTANT)

public:
    ComboBoxPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : EditPreference(category, name, getter, setter) { }
    Type getType() override { return ComboBox; }
 
    const QStringList& getItems() { return _items; }
    void setItems(const QStringList& items) { _items = items; }

protected:
    QStringList _items;
};

class BrowsePreference : public EditPreference {
    Q_OBJECT
    Q_PROPERTY(QString browseLabel READ getBrowseLabel CONSTANT)

public:
    BrowsePreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : EditPreference(category, name, getter, setter) { }
    Type getType() override { return Browsable; }

    const QString& getBrowseLabel() { return _browseLabel; }
    void setBrowseLabel(const QString& browseLabel) { _browseLabel = browseLabel; }

protected:
    QString _browseLabel { "Browse" };
};

class AvatarPreference : public BrowsePreference {
    Q_OBJECT
public:
    AvatarPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : BrowsePreference(category, name, getter, setter) {
        _browseLabel = "Change";
    }
    Type getType() override { return Avatar; }
};


class CheckPreference : public BoolPreference {
    Q_OBJECT
public:
    CheckPreference(const QString& category, const QString& name, Getter getter, Setter setter)
        : BoolPreference(category, name, getter, setter) { }
    Type getType() override { return Checkbox; }
};

#endif


