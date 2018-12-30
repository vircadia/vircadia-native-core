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
#include "ui/overlays/Overlay.h"

/**jsdoc
 * The Keyboard API provides facilities to use 3D Physical keyboard.
 * @namespace Keyboard
 *
 * @hifi-interface
 * @hifi-client-entity
 *
 * @property {bool} raised - <code>true</code> If the keyboard is visible <code>false</code> otherwise
 * @property {bool} password - <code>true</code> Will show * instead of characters in the text display <code>false</code> otherwise
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
    Q_INVOKABLE void loadKeyboardFile(const QString& string);
    Q_INVOKABLE void enableLeftMallet();
    Q_INVOKABLE void enableRightMallet();
    Q_INVOKABLE void disableLeftMallet();
    Q_INVOKABLE void disableRightMallet();
    Q_INVOKABLE void setLeftHandLaser(unsigned int leftHandLaser);
    Q_INVOKABLE void setRightHandLaser(unsigned int rightHandLaser);
    Q_INVOKABLE bool containsID(OverlayID overlayID) const;
private:
    bool getPreferMalletsOverLasers() const;
    bool isRaised() const;
    void setRaised(bool raised);

    bool isPassword() const;
    void setPassword(bool password);

    bool getUse3DKeyboard() const;
};
#endif
