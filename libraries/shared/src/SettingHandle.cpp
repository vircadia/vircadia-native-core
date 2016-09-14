//
//  SettingHandle.h
//
//
//  Created by AndrewMeadows 2015.10.05
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SettingHandle.h"
#include "SettingManager.h"

#include <math.h>



const QString Settings::firstRun { "firstRun" };


Settings::Settings() : _manager(DependencyManager::get<Setting::Manager>())
{
}

QString Settings::fileName() const {
    return _manager->fileName();
}

void Settings::remove(const QString& key) {
    if (key == "" || _manager->contains(key)) {
        _manager->remove(key);
    }
}

QStringList Settings::childGroups() const {
    return _manager->childGroups();
}

QStringList Settings::childKeys() const {
    return _manager->childKeys();
}

QStringList Settings::allKeys() const {
    return _manager->allKeys();
}

bool Settings::contains(const QString& key) const {
    return _manager->contains(key);
}

int Settings::beginReadArray(const QString & prefix) {
    return _manager->beginReadArray(prefix);
}

void Settings::beginWriteArray(const QString& prefix, int size) {
    _manager->beginWriteArray(prefix, size);
}

void Settings::endArray() {
    _manager->endArray();
}

void Settings::setArrayIndex(int i) {
    _manager->setArrayIndex(i);
}

void Settings::beginGroup(const QString& prefix) {
    _manager->beginGroup(prefix);
}

void Settings::endGroup() {
    _manager->endGroup();
}

void Settings::setValue(const QString& name, const QVariant& value) {
    _manager->setValue(name, value);
}

QVariant Settings::value(const QString& name, const QVariant& defaultValue) const {
    return _manager->value(name, defaultValue);
}


void Settings::getFloatValueIfValid(const QString& name, float& floatValue) {
    const QVariant badDefaultValue = NAN;
    bool ok = true;
    float tempFloat = value(name, badDefaultValue).toFloat(&ok);
    if (ok && !glm::isnan(tempFloat)) {
        floatValue = tempFloat;
    }
}

void Settings::getBoolValue(const QString& name, bool& boolValue) {
    const QVariant defaultValue = false;
    boolValue = value(name, defaultValue).toBool();
}


void Settings::setVec3Value(const QString& name, const glm::vec3& vecValue) {
    beginGroup(name);
    {
        setValue(QString("x"), vecValue.x);
        setValue(QString("y"), vecValue.y);
        setValue(QString("z"), vecValue.z);
    }
    endGroup();
}

void Settings::getVec3ValueIfValid(const QString& name, glm::vec3& vecValue) {
    beginGroup(name);
    {
        bool ok = true;
        const QVariant badDefaultValue = NAN;
        float x = value(QString("x"), badDefaultValue).toFloat(&ok);
        float y = value(QString("y"), badDefaultValue).toFloat(&ok);
        float z = value(QString("z"), badDefaultValue).toFloat(&ok);
        if (ok && (!glm::isnan(x) && !glm::isnan(y) && !glm::isnan(z))) {
            vecValue = glm::vec3(x, y, z);
        }
    }
    endGroup();
}

void Settings::setQuatValue(const QString& name, const glm::quat& quatValue) {
    beginGroup(name);
    {
        setValue(QString("x"), quatValue.x);
        setValue(QString("y"), quatValue.y);
        setValue(QString("z"), quatValue.z);
        setValue(QString("w"), quatValue.w);
    }
    endGroup();
}

void Settings::getQuatValueIfValid(const QString& name, glm::quat& quatValue) {
    beginGroup(name);
    {
        bool ok = true;
        const QVariant badDefaultValue = NAN;
        float x = value(QString("x"), badDefaultValue).toFloat(&ok);
        float y = value(QString("y"), badDefaultValue).toFloat(&ok);
        float z = value(QString("z"), badDefaultValue).toFloat(&ok);
        float w = value(QString("w"), badDefaultValue).toFloat(&ok);
        if (ok && (!glm::isnan(x) && !glm::isnan(y) && !glm::isnan(z) && !glm::isnan(w))) {
            quatValue = glm::quat(w, x, y, z);
        }
    }
    endGroup();
}

