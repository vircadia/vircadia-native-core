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

#include <math.h>


void Settings::getFloatValueIfValid(const QString& name, float& floatValue) {
    const QVariant badDefaultValue = NAN;
    bool ok = true;
    float tempFloat = value(name, badDefaultValue).toFloat(&ok);
    if (ok && !isnan(tempFloat)) {
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
        if (ok && (!isnan(x) && !isnan(y) && !isnan(z))) {
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
        if (ok && (!isnan(x) && !isnan(y) && !isnan(z) && !isnan(w))) {
            quatValue = glm::quat(w, x, y, z);
        }
    }
    endGroup();
}

