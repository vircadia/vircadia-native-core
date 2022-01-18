//
//  Created by Bradley Austin Davis 2015/06/19
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarInputs_h
#define hifi_AvatarInputs_h

#include <QQuickItem>
#include <OffscreenUi.h>

#define AI_PROPERTY(type, name, initialValue) \
    Q_PROPERTY(type name READ name NOTIFY name##Changed) \
public: \
    type name() { return _##name; }; \
private: \
    type _##name{ initialValue };

class AvatarInputs : public QObject {
    Q_OBJECT
    HIFI_QML_DECL

    /*@jsdoc 
     * The <code>AvatarInputs</code> API provides facilities to manage user inputs.
     *
     * @namespace AvatarInputs
     *
     * @hifi-interface
     * @hifi-client-entity
     * @hifi-avatar
     *
     * @property {boolean} cameraEnabled - <code>true</code> if webcam face tracking is enabled, <code>false</code> if it is 
     *     disabled.
     *     <em>Read-only.</em>
     *     <p class="important">Deprecated: This property is deprecated and has been removed.</p>
     * @property {boolean} cameraMuted - <code>true</code> if webcam face tracking is muted (temporarily disabled), 
     *     <code>false</code> it if isn't.
     *     <em>Read-only.</em>
     *     <p class="important">Deprecated: This property is deprecated and has been removed.</p>
     * @property {boolean} ignoreRadiusEnabled - <code>true</code> if the privacy shield is enabled, <code>false</code> if it
     *     is disabled.
     *     <em>Read-only.</em>
     * @property {boolean} isHMD - <code>true</code> if the display mode is HMD, <code>false</code> if it isn't. 
     *     <em>Read-only.</em>
     * @property {boolean} showAudioTools - <code>true</code> if the microphone mute button and audio level meter are shown, 
     *     <code>false</code> if they are hidden.
     * @property {boolean} showBubbleTools - <code>true</code> if the privacy shield UI button is shown, <code>false</code> if 
     *     it is hidden.
     */

    AI_PROPERTY(bool, isHMD, false)

    Q_PROPERTY(bool showAudioTools READ showAudioTools WRITE setShowAudioTools NOTIFY showAudioToolsChanged)
    Q_PROPERTY(bool showBubbleTools READ showBubbleTools WRITE setShowBubbleTools NOTIFY showBubbleToolsChanged)
    Q_PROPERTY(bool ignoreRadiusEnabled READ getIgnoreRadiusEnabled NOTIFY ignoreRadiusEnabledChanged)
    //Q_PROPERTY(bool enteredIgnoreRadius READ getEnteredIgnoreRadius NOTIFY enteredIgnoreRadiusChanged)

public:
    static AvatarInputs* getInstance();

    /*@jsdoc
     * Converts non-linear audio loudness to a linear audio level.
     * @function AvatarInputs.loudnessToAudioLevel
     * @param {number} loudness - The non-linear audio loudness.
     * @returns {number} The linear audio level.
     */
    Q_INVOKABLE float loudnessToAudioLevel(float loudness);

    AvatarInputs(QObject* parent = nullptr);
    void update();
    bool showAudioTools() const { return _showAudioTools; }
    bool showBubbleTools() const { return _showBubbleTools; }
    bool getIgnoreRadiusEnabled() const;
    //bool getEnteredIgnoreRadius() const;

public slots:

    /*@jsdoc
     * Sets whether or not the microphone mute button and audio level meter is shown.
     * @function AvatarInputs.setShowAudioTools
     * @param {boolean} showAudioTools - <code>true</code> to show the microphone mute button and audio level meter, 
     *     <code>false</code> to hide it.
     */
    void setShowAudioTools(bool showAudioTools);

    /*@jsdoc
     * Sets whether or not the privacy shield button is shown.
     * @function AvatarInputs.setShowBubbleTools
     * @param {boolean} showBubbleTools - <code>true</code> to show the privacy shield button, <code>false</code> to hide it.
     */
    void setShowBubbleTools(bool showBubbleTools);

signals:

    /*@jsdoc
     * Triggered when webcam face tracking is enabled or disabled.
     * @deprecated This signal is deprecated and has been removed.
     * @function AvatarInputs.cameraEnabledChanged
     * @returns {Signal}
     */

    /*@jsdoc
     * Triggered when webcam face tracking is muted (temporarily disabled) or unmuted.
     * @deprecated This signal is deprecated and has been removed.
     * @function AvatarInputs.cameraMutedChanged
     * @returns {Signal}
     */

    /*@jsdoc
     * Triggered when the display mode changes between desktop and HMD.
     * @function AvatarInputs.isHMDChanged
     * @returns {Signal}
     */
    void isHMDChanged();

    /*@jsdoc
     * Triggered when the visibility of the microphone mute button and audio level meter changes.
     * @function AvatarInputs.showAudioToolsChanged
     * @param {boolean} show - <code>true</code> if the microphone mute button and audio level meter are shown, 
     *     <code>false</code> if they are is hidden.
     * @returns {Signal}
     */
    void showAudioToolsChanged(bool show);

    /*@jsdoc
     * Triggered when the visibility of the privacy shield button changes.
     * @function AvatarInputs.showBubbleToolsChanged
     * @param {boolean} show - <code>true</code> if the privacy shield UI button is shown, <code>false</code> if 
     *     it is hidden.
     * @returns {Signal}
     */
    void showBubbleToolsChanged(bool show);

    /*@jsdoc
     * Triggered when another user enters the privacy shield.
     * @function AvatarInputs.avatarEnteredIgnoreRadius
     * @param {QUuid} avatarID - The session ID of the user that entered the privacy shield.
     * @returns {Signal}
     * @example <caption>Report when a user enters the privacy shield.</caption>
     * AvatarInputs.avatarEnteredIgnoreRadius.connect(function(avatarID) {
     *     print("User entered the privacy shield: " + avatarID);
     * };
     */
    void avatarEnteredIgnoreRadius(QUuid avatarID);

    /*@jsdoc
     * Triggered when another user leaves the privacy shield.
     * <p><strong>Note:</strong> Currently doesn't work.</p>
     * @function AvatarInputs.avatarLeftIgnoreRadius
     * @param {QUuid} avatarID - The session ID of the user that exited the privacy shield.
     * @returns {Signal}
     * @deprecated This signal is deprecated and will be removed.
     */
    void avatarLeftIgnoreRadius(QUuid avatarID);

    /*@jsdoc
     * Triggered when the privacy shield is enabled or disabled.
     * @function AvatarInputs.ignoreRadiusEnabledChanged
     * @param {boolean} enabled - <code>true</code> if the privacy shield is enabled, <code>false</code> if it is disabled.
     * @returns {Signal}
     */
    void ignoreRadiusEnabledChanged(bool enabled);

    /*@jsdoc
     * Triggered when another user enters the privacy shield.
     * @function AvatarInputs.enteredIgnoreRadiusChanged
     * @returns {Signal}
     */
    void enteredIgnoreRadiusChanged();

protected:

    /*@jsdoc
     * Resets sensors, audio, avatar animations, and the avatar rig.
     * @function AvatarInputs.resetSensors
     */
    Q_INVOKABLE void resetSensors();

    /*@jsdoc
     * Toggles the muting (temporary disablement) of webcam face tracking on/off.
     * <p class="important">Deprecated: This function is deprecated and has been removed.</p>
     * @function AvatarInputs.toggleCameraMute
     */

private: 
    void onAvatarEnteredIgnoreRadius();
    void onAvatarLeftIgnoreRadius();
    float _trailingAudioLoudness{ 0 };
    bool _showAudioTools { true };
    bool _showBubbleTools{ true };
};

#endif // hifi_AvatarInputs_h
