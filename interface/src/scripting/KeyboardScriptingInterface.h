//
//  KeyboardScriptingInterface.h
//  interface/src/scripting
//
//  Created by Dante Ruiz on 2018-08-27.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_KeyboardScriptingInterface_h
#define hifi_KeyboardScriptingInterface_h

#include <QtCore/QObject>
#include <QtCore/QUuid>

#include "DependencyManager.h"

/*@jsdoc
 * The <code>Keyboard</code> API provides facilities to use an in-world, virtual keyboard. When enabled, this keyboard is 
 * displayed instead of the 2D keyboard that raises at the bottom of the tablet or Web entities when a text input field has 
 * focus and you're in HMD mode.
 *
 * @namespace Keyboard
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 *
 * @property {boolean} raised - <code>true</code> if the virtual keyboard is visible, <code>false</code> if it isn't.
 * @property {boolean} password - <code>true</code> if <code>"*"</code>s are displayed on the virtual keyboard's display 
 *     instead of the characters typed, <code>false</code> if the actual characters are displayed.
 * @property {boolean} use3DKeyboard - <code>true</code> if user settings have "Use Virtual Keyboard" enabled, 
 *     <code>false</code> if it's disabled. <em>Read-only.</em>
 * @property {boolean} preferMalletsOverLasers - <code>true</code> if user settings for the virtual keyboard have "Mallets" 
 *     selected, <code>false</code> if "Lasers" is selected. <em>Read-only.</em>
 */

class KeyboardScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    Q_PROPERTY(bool raised READ isRaised WRITE setRaised)
    Q_PROPERTY(bool password READ isPassword WRITE setPassword)
    Q_PROPERTY(bool use3DKeyboard READ getUse3DKeyboard CONSTANT);
    Q_PROPERTY(bool preferMalletsOverLasers READ getPreferMalletsOverLasers CONSTANT)

public:
    KeyboardScriptingInterface() = default;
    ~KeyboardScriptingInterface() = default;

    /*@jsdoc
     * Loads a JSON file that defines the virtual keyboard's layout. The default JSON file used is 
     * {@link https://github.com/highfidelity/hifi/blob/master/interface/resources/config/keyboard.json|https://github.com/highfidelity/hifi/.../keyboard.json}.
     * @function Keyboard.loadKeyboardFile
     * @param {string} path - The keyboard JSON file.
     */
    Q_INVOKABLE void loadKeyboardFile(const QString& string);

    /*@jsdoc
     * Enables the left mallet so that it is displayed when in HMD mode.
     * @function Keyboard.enableLeftMallet
     */
    Q_INVOKABLE void enableLeftMallet();

    /*@jsdoc
     * Enables the right mallet so that it is displayed when in HMD mode.
     * @function Keyboard.enableRightMallet
     */
    Q_INVOKABLE void enableRightMallet();

    /*@jsdoc
     * Disables the left mallet so that it is not displayed when in HMD mode.
     * @function Keyboard.disableLeftMallet
     */
    Q_INVOKABLE void disableLeftMallet();

    /*@jsdoc
     * Disables the right mallet so that it is not displayed when in HMD mode.
     * @function Keyboard.disableRightMallet
     */
    Q_INVOKABLE void disableRightMallet();

    /*@jsdoc
     * Configures the virtual keyboard to recognize a ray pointer as the left hand's laser.
     * @function Keyboard.setLeftHandLaser
     * @param {number} leftHandLaser - The ID of a ray pointer created by {@link Pointers.createPointer}.
     */
    Q_INVOKABLE void setLeftHandLaser(unsigned int leftHandLaser);

    /*@jsdoc
     * Configures the virtual keyboard to recognize a ray pointer as the right hand's laser.
     * @function Keyboard.setRightHandLaser
     * @param {number} rightHandLaser - The ID of a ray pointer created by {@link Pointers.createPointer}.
     */
    Q_INVOKABLE void setRightHandLaser(unsigned int rightHandLaser);

    /*@jsdoc
     * Checks whether an entity is part of the virtual keyboard.
     * @function Keyboard.containsID
     * @param {Uuid} entityID - The entity ID.
     * @returns {boolean} <code>true</code> if the entity is part of the virtual keyboard, <code>false</code> if it isn't.
     */
    Q_INVOKABLE bool containsID(const QUuid& overlayID) const;

private:
    bool getPreferMalletsOverLasers() const;
    bool isRaised() const;
    void setRaised(bool raised);

    bool isPassword() const;
    void setPassword(bool password);

    bool getUse3DKeyboard() const;
};
#endif
