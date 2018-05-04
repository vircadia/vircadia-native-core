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
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-assignment-client
 *
 * @property {boolean} canKick - <code>true</code> if the domain server allows the node or avatar to kick (ban) avatars,
 *     otherwise <code>false</code>. <em>Read-only.</em>
 * @property {boolean} requestsDomainListData - <code>true</code> if the avatar requests extra data from the mixers (such as 
 *     positional data of an avatar you've ignored). <em>Read-only.</em>
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
     * Personally ignore another user, making them disappear for you and you disappear for them. 
     * @function Users.ignore
     * @param {Uuid} nodeID The node or session ID of the user you want to ignore.
     * @param {boolean} enable True for ignored; false for un-ignored.
     */
    void ignore(const QUuid& nodeID, bool ignoreEnabled = true);

    /**jsdoc
     * Get whether or not you have ignored the node with the given UUID.
     * @function Users.getIgnoreStatus
     * @param {Uuid} nodeID The node or session ID of the user whose ignore status you want.
     * @returns {boolean}
     */
    bool getIgnoreStatus(const QUuid& nodeID);

    /**jsdoc
     * Mute another user for you and you only. They won't be able to hear you, and you won't be able to hear them.
     * @function Users.personalMute
     * @param {Uuid} nodeID The node or session ID of the user you want to mute.
     * @param {boolean} muteEnabled True for enabled; false for disabled.
     */
    void personalMute(const QUuid& nodeID, bool muteEnabled = true);

    /**jsdoc
      * Get whether or not you have personally muted the node with the given UUID.
      * @function Users.requestPersonalMuteStatus
      * @param {Uuid} nodeID The node or session ID of the user whose personal mute status you want.
      * @returns {boolean}
      */
    bool getPersonalMuteStatus(const QUuid& nodeID);

    /**jsdoc
     * Sets an avatar's gain for you and you only.
     * Units are Decibels (dB)
     * @function Users.setAvatarGain
     * @param {Uuid} nodeID The node or session ID of the user whose gain you want to modify, or null to set the master gain.
     * @param {number} gain The gain of the avatar you'd like to set. Units are dB.
    */
    void setAvatarGain(const QUuid& nodeID, float gain);

    /**jsdoc
     * Gets an avatar's gain for you and you only.
     * @function Users.getAvatarGain
     * @param {Uuid} nodeID The node or session ID of the user whose gain you want to get, or null to get the master gain.
     * @returns {number} gain (in dB)
    */
    float getAvatarGain(const QUuid& nodeID);

    /**jsdoc
     * Kick/ban another user. Removes them from the server and prevents them from returning. Bans by either user name (if 
     * available) or machine fingerprint otherwise. This will only do anything if you're an admin of the domain you're in. 
     * @function Users.kick
     * @param {Uuid} nodeID The node or session ID of the user you want to kick.
     */
    void kick(const QUuid& nodeID);

    /**jsdoc
     * Mutes another user's microphone for everyone. Not permanent; the silenced user can unmute themselves with the UNMUTE 
     * button in their HUD. This will only do anything if you're an admin of the domain you're in. 
     * @function Users.mute
     * @param {Uuid} nodeID The node or session ID of the user you want to mute.
     */
    void mute(const QUuid& nodeID);

    /**jsdoc
    * Get the user name and machine fingerprint associated with the given UUID. This will only do anything if you're an admin 
    * of the domain you're in.
    * @function Users.getUsernameFromID
    * @param {Uuid} nodeID The node or session ID of the user whose username you want.
    */
    void requestUsernameFromID(const QUuid& nodeID);

    /**jsdoc
     * Returns `true` if the DomainServer will allow this Node/Avatar to make kick.
     * @function Users.getCanKick
     * @returns {boolean} <code>true</code> if the domain server allows the client to kick (ban) other users, otherwise 
     *     <code>false</code>.
     */
    bool getCanKick();

    /**jsdoc
     * Toggle the state of the space bubble feature.
     * @function Users.toggleIgnoreRadius
     */
    void toggleIgnoreRadius();

    /**jsdoc
     * Enables the space bubble feature.
     * @function Users.enableIgnoreRadius
     */
    void enableIgnoreRadius();

    /**jsdoc
     * Disables the space bubble feature.
     * @function Users.disableIgnoreRadius
     */
    void disableIgnoreRadius();

    /**jsdoc
     * Returns `true` if the space bubble feature is enabled.
     * @function Users.getIgnoreRadiusEnabled
     * @returns {boolean} <code>true</code> if the space bubble is enabled, otherwise <code>false</code>.
     */
    bool getIgnoreRadiusEnabled();

signals:
    
    /**jsdoc
     * @function Users.canKickChanged
     * @param {boolean} canKick
     * @returns {Signal}
     */
    void canKickChanged(bool canKick);

    /**jsdoc
     * @function Users.ignoreRadiusEnabledChanged
     * @param {boolean} isEnabled
     * @returns {Signal}
     */
    void ignoreRadiusEnabledChanged(bool isEnabled);

    /**jsdoc
     * Notifies scripts that another user has entered the ignore radius.
     * @function Users.enteredIgnoreRadius
     * @returns {Signal}
     */
    void enteredIgnoreRadius();

    /**jsdoc
     * Notifies scripts of the user name and machine fingerprint associated with a UUID.
     * Username and machineFingerprint will be their default constructor output if the requesting user isn't an admin.
     * @function Users.usernameFromIDReply
     * @param {Uuid} nodeID
     * @param {string} userName
     * @param {string} machineFingerprint
     * @param {boolean} isAdmin
     * @returns {Signal}
    */
    void usernameFromIDReply(const QString& nodeID, const QString& username, const QString& machineFingerprint, bool isAdmin);

    /**jsdoc
     * Notifies scripts that a user has disconnected from the domain.
     * @function Users.avatarDisconnected
     * @param {Uuid} nodeID The session ID of the avatar that has disconnected.
     * @returns {Signal}
     */
    void avatarDisconnected(const QUuid& nodeID);

private:
    bool getRequestsDomainListData();
    void setRequestsDomainListData(bool requests);
};


#endif // hifi_UsersScriptingInterface_h
