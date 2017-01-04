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
    Q_PROPERTY(bool requestsDomainListData READ getRequestsDomainListData WRITE setRequestsDomainListData)

public:
    UsersScriptingInterface();

public slots:

    /**jsdoc
    * Ignore another user.
    * @function Users.ignore
    * @param {nodeID} nodeID The node or session ID of the user you want to ignore.
    * @param {bool} enable True for ignored; false for un-ignored.
    */
    void ignore(const QUuid& nodeID, bool ignoreEnabled);

    /**jsdoc
    * Gets a bool containing whether you have ignored the given Avatar UUID.
    * @function Users.getIgnoreStatus
    * @param {nodeID} nodeID The node or session ID of the user whose ignore status you want.
    */
    bool getIgnoreStatus(const QUuid& nodeID);

    /**jsdoc
    * Mute another user for you and you only.
    * @function Users.personalMute
    * @param {nodeID} nodeID The node or session ID of the user you want to mute.
    * @param {bool} enable True for enabled; false for disabled.
    */
    void personalMute(const QUuid& nodeID, bool muteEnabled);

    /**jsdoc
    * Requests a bool containing whether you have personally muted the given Avatar UUID.
    * @function Users.requestPersonalMuteStatus
    * @param {nodeID} nodeID The node or session ID of the user whose personal mute status you want.
    */
    bool getPersonalMuteStatus(const QUuid& nodeID);

    /**jsdoc
    * Kick another user.
    * @function Users.kick
    * @param {nodeID} nodeID The node or session ID of the user you want to kick.
    */
    void kick(const QUuid& nodeID);

    /**jsdoc
    * Mute another user for everyone.
    * @function Users.mute
    * @param {nodeID} nodeID The node or session ID of the user you want to mute.
    */
    void mute(const QUuid& nodeID);

    /**jsdoc
    * Returns a string containing the username associated with the given Avatar UUID
    * @function Users.getUsernameFromID
    * @param {nodeID} nodeID The node or session ID of the user whose username you want.
    */
    void requestUsernameFromID(const QUuid& nodeID);

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
    * Returns `true` if the ignore in radius feature is enabled
    * @function Users.getIgnoreRadiusEnabled
    * @return {bool} `true` if the ignore in radius feature is enabled, `false` if not.
    */
    bool getIgnoreRadiusEnabled();

signals:
    void canKickChanged(bool canKick);
    void ignoreRadiusEnabledChanged(bool isEnabled);
    void ignoredNode(const QUuid& nodeID, bool enabled);

    /**jsdoc
    * Notifies scripts that another user has entered the ignore radius
    * @function Users.enteredIgnoreRadius
    */
    void enteredIgnoreRadius();

    /**jsdoc
    * Notifies scripts of the username and machine fingerprint associated with a UUID.
    * @function Users.usernameFromIDReply
    */
    void usernameFromIDReply(const QString& nodeID, const QString& username, const QString& machineFingerprint);

private:
    bool getRequestsDomainListData();
    void setRequestsDomainListData(bool requests);
    bool _requestsDomainListData;
};


#endif // hifi_UsersScriptingInterface_h
