//
//  ScriptUUID.h
//  libraries/script-engine/src/
//
//  Created by Andrew Meadows on 2014-04-07
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable interface for a UUID helper class object. Used exclusively in the JavaScript API
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ScriptUUID_h
#define hifi_ScriptUUID_h

#include <QUuid>
#include <QtScript/QScriptable>

/**jsdoc
 * A UUID (Universally Unique IDentifier) is used to uniquely identify entities, overlays, avatars, and the like. It is
 * represented in JavaScript as a string in the format, <code>{nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}</code>, where the "n"s are
 * hexadecimal digits.
 *
 * @namespace Uuid
 * @property NULL {Uuid} The null UUID, <code>{00000000-0000-0000-0000-000000000000}</code>.
 */

/// Scriptable interface for a UUID helper class object. Used exclusively in the JavaScript API
class ScriptUUID : public QObject, protected QScriptable {
    Q_OBJECT
    Q_PROPERTY(QString NULL READ NULL_UUID CONSTANT) // String for use in scripts.

public slots:
    /**jsdoc
     * Generates a UUID from a string representation of the UUID.
     * @function Uuid.fromString
     * @param {string} string - A string representation of a UUID. The curly braces are optional.
     * @returns {Uuid} A UUID if the given <code>string</code> is valid, <code>null</code> otherwise.
     * @example <caption>Valid and invalid parameters.</caption>
     * var uuid = Uuid.fromString("{527c27ea-6d7b-4b47-9ae2-b3051d50d2cd}");
     * print(uuid); // {527c27ea-6d7b-4b47-9ae2-b3051d50d2cd}
     *
     * uuid = Uuid.fromString("527c27ea-6d7b-4b47-9ae2-b3051d50d2cd");
     * print(uuid); // {527c27ea-6d7b-4b47-9ae2-b3051d50d2cd}
     *
     * uuid = Uuid.fromString("527c27ea");
     * print(uuid); // null
     */
    QUuid fromString(const QString& string);

    /**jsdoc
     * Generates a string representation of a UUID. However, because UUIDs are represented in JavaScript as strings, this is in
     * effect a no-op.
     * @function Uuid.toString
     * @param {Uuid} id - The UUID to generate a string from.
     * @returns {string} - A string representation of the UUID.
     */
    QString toString(const QUuid& id);
    
    /**jsdoc
     * Generate a new UUID.
     * @function Uuid.generate
     * @returns {Uuid} A new UUID.
     * @example <caption>Generate a new UUID and reports its JavaScript type.</caption>
     * var uuid = Uuid.generate();
     * print(uuid);        // {nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}
     * print(typeof uuid); // string
     */
    QUuid generate();

    /**jsdoc
     * Test whether two given UUIDs are equal.
     * @function Uuid.isEqual
     * @param {Uuid} idA - The first UUID to compare.
     * @param {Uuid} idB - The second UUID to compare.
     * @returns {boolean} <code>true</code> if the two UUIDs are equal, otherwise <code>false</code>.
     * @example <caption>Demonstrate <code>true</code> and <code>false</code> cases.</caption>
     * var uuidA = Uuid.generate();
     * var uuidB = Uuid.generate();
     * print(Uuid.isEqual(uuidA, uuidB)); // false
     * uuidB = uuidA;
     * print(Uuid.isEqual(uuidA, uuidB)); // true
     */
    bool isEqual(const QUuid& idA, const QUuid& idB);

    /**jsdoc
     * Test whether a given UUID is null.
     * @function Uuid.isNull
     * @param {Uuid} id - The UUID to test.
     * @returns {boolean} <code>true</code> if the UUID equals Uuid.NULL or is <code>null</code>, otherwise <code>false</code>.
     * @example <caption>Demonstrate <code>true</code> and <code>false</code> cases.</caption>
     * var uuid; // undefined
     * print(Uuid.isNull(uuid)); // false
     * uuid = Uuid.generate();
     * print(Uuid.isNull(uuid)); // false
     * uuid = Uuid.NULL;
     * print(Uuid.isNull(uuid)); // true
     * uuid = null;
     * print(Uuid.isNull(uuid)); // true
     */
    bool isNull(const QUuid& id);

    /**jsdoc
     * Print to the program log a text label followed by the UUID value.
     * @function Uuid.print
     * @param {string} label - The label to print.
     * @param {Uuid} id - The UUID to print.
     * @example <caption>Two ways of printing a label plus UUID.</caption>
     * var uuid = Uuid.generate();
     * Uuid.print("Generated UUID:", uuid); // Generated UUID: {nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}
     * print("Generated UUID: " + uuid);    // Generated UUID: {nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn}
     */
    void print(const QString& label, const QUuid& id);

private:
    const QString NULL_UUID() { return NULL_ID; }
    const QString NULL_ID { "{00000000-0000-0000-0000-000000000000}" };
};

#endif // hifi_ScriptUUID_h
