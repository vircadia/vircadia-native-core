//
//  UsersScriptingInterface.h
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-07-11.
//  Copyright 2016 High Fidelity, Inc.
//  Copyright 2021 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/// @addtogroup ScriptEngine
/// @{

#pragma once

#ifndef hifi_UsersScriptingInterface_h
#define hifi_UsersScriptingInterface_h

#include <DependencyManager.h>
#include <shared/ReadWriteLockable.h>
#include <ModerationFlags.h>

/*@jsdoc
 * The <code>Users</code> API provides features to regulate your interaction with other users.
 *
 * @namespace Users
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 * @hifi-assignment-client
 *
 * @property {boolean} canKick - <code>true</code> if the domain server allows the client to kick (ban) avatars, otherwise 
 *     <code>false</code>. <em>Read-only.</em>
 * @property {boolean} requestsDomainListData - <code>true</code> if the client requests extra data from the mixers (such as 
 *     positional data of an avatar they've ignored). <em>Read-only.</em>
 * @property {BanFlags} NO_BAN - Do not ban user. <em>Read-only.</em>
 * @property {BanFlags} BAN_BY_USERNAME - Ban user by username. <em>Read-only.</em>
 * @property {BanFlags} BAN_BY_FINGERPRINT - Ban user by fingerprint. <em>Read-only.</em>
 * @property {BanFlags} BAN_BY_IP - Ban user by IP address. <em>Read-only.</em>
 */
/// Provides the <code><a href="https://apidocs.vircadia.dev/Users.html">Users</a></code> scripting interface
class UsersScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
    SINGLETON_DEPENDENCY

    Q_PROPERTY(bool canKick READ getCanKick)
    Q_PROPERTY(bool requestsDomainListData READ getRequestsDomainListData WRITE setRequestsDomainListData)

    Q_PROPERTY(unsigned int NO_BAN READ getNoBan CONSTANT)
    Q_PROPERTY(unsigned int BAN_BY_USERNAME READ getBanByUsername CONSTANT)
    Q_PROPERTY(unsigned int BAN_BY_FINGERPRINT READ getBanByFingerprint CONSTANT)
    Q_PROPERTY(unsigned int BAN_BY_IP READ getBanByIP CONSTANT)

public:
    UsersScriptingInterface();
    void setKickConfirmationOperator(std::function<void(const QUuid& nodeID, unsigned int banFlags)> kickConfirmationOperator) {
        _kickConfirmationOperator = kickConfirmationOperator;
    }

    bool getWaitForKickResponse() { return _kickResponseLock.resultWithReadLock<bool>([&] { return _waitingForKickResponse; }); }
    void setWaitForKickResponse(bool waitForKickResponse) { _kickResponseLock.withWriteLock([&] { _waitingForKickResponse = waitForKickResponse; }); }

public slots:

    /*@jsdoc
     * Ignores or un-ignores another avatar. Ignoring an avatar makes them disappear for you and you disappear for them. 
     * @function Users.ignore
     * @param {Uuid} sessionID - The session ID of the avatar to ignore.
     * @param {boolean} [enable=true] - <code>true</code> to ignore, <code>false</code> to un-ignore.
     * @example <caption>Ignore a nearby avatar for a few seconds.</caption>
     * var avatars = AvatarList.getAvatarsInRange(MyAvatar.position, 1000);
     * if (avatars.length > 1) {  // Skip own avatar which is provided in position 0.
     *     print("Ignore: " + avatars[1]);
     *     Users.ignore(avatars[1], true);
     *     Script.setTimeout(function () {
     *         print("Un-ignore: " + avatars[1]);
     *         Users.ignore(avatars[1], false);
     *     }, 5000);
     * } else {
     *     print("No avatars");
     * }
     */
    void ignore(const QUuid& nodeID, bool ignoreEnabled = true);

    /*@jsdoc
     * Gets whether or not you have ignored a particular avatar.
     * @function Users.getIgnoreStatus
     * @param {Uuid} sessionID - The session ID of the avatar to get the ignore status of.
     * @returns {boolean} <code>true</code> if the avatar is being ignored, <code>false</code> if it isn't.
     */
    bool getIgnoreStatus(const QUuid& nodeID);

    /*@jsdoc
     * Mutes or un-mutes another avatar. Muting makes you unable to hear them and them unable to hear you.
     * @function Users.personalMute
     * @param {Uuid} sessionID - The session ID of the avatar to mute.
     * @param {boolean} [muteEnabled=true] - <code>true</code> to mute, <code>false</code> to un-mute.
     */
    void personalMute(const QUuid& nodeID, bool muteEnabled = true);

    /*@jsdoc
      * Gets whether or not you have muted a particular avatar.
      * @function Users.getPersonalMuteStatus
      * @param {Uuid} sessionID - The session ID of the avatar to get the mute status of.
      * @returns {boolean} <code>true</code> if the avatar is muted, <code>false</code> if it isn't.
      */
    bool getPersonalMuteStatus(const QUuid& nodeID);

    /*@jsdoc
     * Sets an avatar's gain (volume) for you and you only, or sets the master gain.
     * @function Users.setAvatarGain
     * @param {Uuid} nodeID - The session ID of the avatar to set the gain for, or <code>null</code> to set the master gain.
     * @param {number} gain - The gain to set, in dB.
    */
    void setAvatarGain(const QUuid& nodeID, float gain);

    /*@jsdoc
     * Gets an avatar's gain (volume) for you and you only, or gets the master gain.
     * @function Users.getAvatarGain
     * @param {Uuid} nodeID - The session ID of the avatar to get the gain for, or <code>null</code> to get the master gain.
     * @returns {number} The gain, in dB.
    */
    float getAvatarGain(const QUuid& nodeID);

    /*@jsdoc
     * Kicks and bans a user. This removes them from the server and prevents them from returning. The ban is by user name (if 
     * available) and by machine fingerprint. The ban functionality can be controlled with flags.
     * <p>This function only works if you're an administrator of the domain you're in.</p>
     * @function Users.kick
     * @param {Uuid} sessionID - The session ID of the user to kick and ban.
     * @param {BanFlags} - Preferred ban flags. <i>Bans a user by username (if available) and machine fingerprint by default.</i>
     */
    void kick(const QUuid& nodeID, unsigned int banFlags = ModerationFlags::getDefaultBanFlags());

    /*@jsdoc
     * Mutes a user's microphone for everyone. The mute is not permanent: the user can unmute themselves. 
     * <p>This function only works if you're an administrator of the domain you're in.</p>
     * @function Users.mute
     * @param {Uuid} sessionID - The session ID of the user to mute.
     */
    void mute(const QUuid& nodeID);

    /*@jsdoc
     * Requests the user name and machine fingerprint associated with the given UUID. The user name is returned via a 
     * {@link Users.usernameFromIDReply|usernameFromIDReply} signal.
     * <p>This function only works if you're an administrator of the domain you're in.</p>
     * @function Users.requestUsernameFromID
     * @param {Uuid} sessionID - The session ID of the user to get the user name and machine fingerprint of.
     * @example <caption>Report the user name and fingerprint of a nearby user.</caption>
     * function onUsernameFromIDReply(sessionID, userName, machineFingerprint, isAdmin) {
     *     print("Session:     " + sessionID);
     *     print("User name:   " + userName);
     *     print("Fingerprint: " + machineFingerprint);
     *     print("Is admin:    " + isAdmin);
     * }
     * 
     * Users.usernameFromIDReply.connect(onUsernameFromIDReply);
     * 
     * var avatars = AvatarList.getAvatarsInRange(MyAvatar.position, 1000);
     * if (avatars.length > 1) {  // Skip own avatar which is provided in position 0.
     *     print("Request data for: " + avatars[1]);
     *     Users.requestUsernameFromID(avatars[1]);
     * } else {
     *     print("No avatars");
     * }
     */
    void requestUsernameFromID(const QUuid& nodeID);

    /*@jsdoc
     * Gets whether the client can kick and ban users in the domain.
     * @function Users.getCanKick
     * @returns {boolean} <code>true</code> if the domain server allows the client to kick and ban users, otherwise 
     *     <code>false</code>.
     */
    bool getCanKick();

    /*@jsdoc
     * Toggles the state of the privacy shield.
     * @function Users.toggleIgnoreRadius
     */
    void toggleIgnoreRadius();

    /*@jsdoc
     * Enables the privacy shield.
     * @function Users.enableIgnoreRadius
     */
    void enableIgnoreRadius();

    /*@jsdoc
     * Disables the privacy shield.
     * @function Users.disableIgnoreRadius
     */
    void disableIgnoreRadius();

    /*@jsdoc
     * Gets the status of the privacy shield.
     * @function Users.getIgnoreRadiusEnabled
     * @returns {boolean} <code>true</code> if the privacy shield is enabled, <code>false</code> if it is disabled.
     */
    bool getIgnoreRadiusEnabled();

signals:
    
    /*@jsdoc
     * Triggered when your ability to kick and ban users changes.
     * @function Users.canKickChanged
     * @param {boolean} canKick - <code>true</code> if you can kick and ban users, <code>false</code> if you can't.
     * @returns {Signal}
     */
    void canKickChanged(bool canKick);

    /*@jsdoc
     * Triggered when the privacy shield status changes.
     * @function Users.ignoreRadiusEnabledChanged
     * @param {boolean} isEnabled - <code>true</code> if the privacy shield is enabled, <code>false</code> if it isn't.
     * @returns {Signal}
     */
    void ignoreRadiusEnabledChanged(bool isEnabled);

    /*@jsdoc
     * Triggered when another user enters the privacy shield.
     * @function Users.enteredIgnoreRadius
     * @returns {Signal}
     */
    void enteredIgnoreRadius();

    /*@jsdoc
     * Triggered in response to a {@link Users.requestUsernameFromID|requestUsernameFromID} call. Provides the user name and 
     * machine fingerprint associated with a UUID.
     * @function Users.usernameFromIDReply
     * @param {Uuid} sessionID - The session ID of the client that the data is for.
     * @param {string} userName - The user name of the client, if the requesting client is an administrator in the domain or 
     *     the <code>sessionID</code> is that of the client, otherwise <code>""</code>.
     * @param {Uuid} machineFingerprint - The machine fingerprint of the client, if the requesting client is an administrator 
     *     in the domain or the <code>sessionID</code> is that of the client, otherwise {@link Uuid|Uuid.NULL}.
     * @param {boolean} isAdmin - <code>true</code> if the client is an administrator in the domain, <code>false</code> if not.
     * @returns {Signal}
    */
    void usernameFromIDReply(const QString& nodeID, const QString& username, const QString& machineFingerprint, bool isAdmin);

    /*@jsdoc
     * Triggered when a client has disconnected from the domain.
     * @function Users.avatarDisconnected
     * @param {Uuid} sessionID - The session ID of the client that has disconnected.
     * @returns {Signal}
     */
    void avatarDisconnected(const QUuid& nodeID);

private:
    bool getRequestsDomainListData();
    void setRequestsDomainListData(bool requests);

    static constexpr unsigned int getNoBan() { return ModerationFlags::BanFlags::NO_BAN; };
    static constexpr unsigned int getBanByUsername() { return ModerationFlags::BanFlags::BAN_BY_USERNAME; };
    static constexpr unsigned int getBanByFingerprint() { return ModerationFlags::BanFlags::BAN_BY_FINGERPRINT; };
    static constexpr unsigned int getBanByIP() { return ModerationFlags::BanFlags::BAN_BY_IP; };

    std::function<void(const QUuid& nodeID, unsigned int banFlags)> _kickConfirmationOperator;

    ReadWriteLockable _kickResponseLock;
    bool _waitingForKickResponse { false };
};


#endif // hifi_UsersScriptingInterface_h

/// @}
