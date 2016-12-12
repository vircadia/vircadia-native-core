//
//  UsersScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-07-11.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once

#ifndef hifi_UsersScriptingInterface_h
#define hifi_UsersScriptingInterface_h

#include <DependencyManager.h>

/**jsdoc
* @namespace Users
*/
class UsersScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    Q_PROPERTY(bool canKick READ getCanKick)

public:
    UsersScriptingInterface();

public slots:

    /**jsdoc
    * Ignore another user.
    * @function Users.ignore
    * @param {nodeID} nodeID The node or session ID of the user you want to ignore.
    */
    void ignore(const QUuid& nodeID);

    /**jsdoc
    * Kick another user.
    * @function Users.kick
    * @param {nodeID} nodeID The node or session ID of the user you want to kick.
    */
    void kick(const QUuid& nodeID);

    /**jsdoc
    * Mute another user.
    * @function Users.mute
    * @param {nodeID} nodeID The node or session ID of the user you want to mute.
    */
    void mute(const QUuid& nodeID);

    /**jsdoc
    * Returns `true` if the DomainServer will allow this Node/Avatar to make kick
    * @function Users.getCanKick
    * @return {bool} `true` if the client can kick other users, `false` if not.
    */
    bool getCanKick();

    /**jsdoc
    * Toggle the state of the ignore in radius feature
    * @function Users.toggleIgnoreRadius
    */
    void toggleIgnoreRadius();

    /**jsdoc
    * Enables the ignore radius feature.
    * @function Users.enableIgnoreRadius
    */
    void enableIgnoreRadius();

    /**jsdoc
    * Disables the ignore radius feature.
    * @function Users.disableIgnoreRadius
    */
    void disableIgnoreRadius();

    /**jsdoc
    * sets the parameters for the ignore radius feature.
    * @function Users.setIgnoreRadius
    * @param {number} radius The radius for the auto ignore in radius feature
    * @param {bool} [enabled=true] Whether the ignore in radius feature should be enabled
    */
    void setIgnoreRadius(float radius, bool enabled = true);

    /**jsdoc
    * Returns the effective radius of the ingore radius feature if it is enabled.
    * @function Users.getIgnoreRadius
    * @return {number} radius of the ignore feature
    */
    float getIgnoreRadius();

    /**jsdoc
    * Returns `true` if the ignore in radius feature is enabled
    * @function Users.getIgnoreRadiusEnabled
    * @return {bool} `true` if the ignore in radius feature is enabled, `false` if not.
    */
    bool getIgnoreRadiusEnabled();

signals:
    void canKickChanged(bool canKick);
    void ignoreRadiusEnabledChanged(bool isEnabled);
};


#endif // hifi_UsersScriptingInterface_h
