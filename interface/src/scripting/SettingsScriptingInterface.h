//
//  SettingsScriptingInterface.h
//  interface/src/scripting
//
//  Created by Brad Hefta-Gaub on 3/22/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SettingsScriptingInterface_h
#define hifi_SettingsScriptingInterface_h

#include <QObject>
#include <QString>

/*@jsdoc
 * The <code>Settings</code> API provides a facility to store and retrieve values that persist between Interface runs.
 *
 * @namespace Settings
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */

class SettingsScriptingInterface : public QObject {
    Q_OBJECT
public:
    static SettingsScriptingInterface* getInstance();

public slots:

    /*@jsdoc
     * Retrieves the value from a named setting.
     * @function Settings.getValue
     * @param {string} key - The name of the setting.
     * @param {string|number|boolean|object} [defaultValue=""] - The value to return if the setting doesn't exist.
     * @returns {string|number|boolean|object} The value stored in the named setting if it exists, otherwise the 
     *     <code>defaultValue</code>.
     * @example <caption>Retrieve non-existent setting values.</caption>
     * var value1 = Settings.getValue("Script Example/Nonexistent Key");
     * print("Value: " + (typeof value1) + " " + JSON.stringify(value1));  // string ""
     *
     * var value2 = Settings.getValue("Script Example/Nonexistent Key", true);
     * print("Value: " + (typeof value2) + " " + JSON.stringify(value2));  // boolean true
     */
    QVariant getValue(const QString& setting);
    QVariant getValue(const QString& setting, const QVariant& defaultValue);

    /*@jsdoc
     * Stores a value in a named setting. If the setting already exists, its value is overwritten. If the value is 
     * <code>null</code> or <code>undefined</code>, the setting is deleted.
     * @function Settings.setValue
     * @param {string} key - The name of the setting. Be sure to use a unique name if creating a new setting.
     * @param {string|number|boolean|object|undefined} value - The value to store in the setting. If <code>null</code> or 
     *     <code>undefined</code> is specified, the setting is deleted.
     * @example <caption>Store and retrieve an object value.</caption>
     * Settings.setValue("Script Example/My Key", { x: 0, y: 10, z: 0 });
     *
     * var value = Settings.getValue("Script Example/My Key");
     * print("Value: " + (typeof value) + " " + JSON.stringify(value));  // object {"x":0,"y":10,"z":0}
     */
    void setValue(const QString& setting, const QVariant& value);

signals:
    void valueChanged(const QString& setting, const QVariant& value);

protected:
    SettingsScriptingInterface(QObject* parent = nullptr) : QObject(parent) { };
    bool _restrictPrivateValues { true };
};

class QMLSettingsScriptingInterface : public SettingsScriptingInterface {
    Q_OBJECT
public:
    QMLSettingsScriptingInterface(QObject* parent) : SettingsScriptingInterface(parent) { _restrictPrivateValues = false; }
};

#endif // hifi_SettingsScriptingInterface_h
