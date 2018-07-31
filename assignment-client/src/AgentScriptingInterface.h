//
//  AgentScriptingInterface.h
//  assignment-client/src
//
//  Created by Thijs Wenker on 7/23/18.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#ifndef hifi_AgentScriptingInterface_h
#define hifi_AgentScriptingInterface_h

#include <QObject>

#include "Agent.h"

/**jsdoc
 * @namespace Agent
 *
 * @hifi-assignment-client
 *
 * @property {boolean} isAvatar
 * @property {boolean} isPlayingAvatarSound <em>Read-only.</em>
 * @property {boolean} isListeningToAudioStream
 * @property {boolean} isNoiseGateEnabled
 * @property {number} lastReceivedAudioLoudness <em>Read-only.</em>
 * @property {Uuid} sessionUUID <em>Read-only.</em>
 */
class AgentScriptingInterface : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isAvatar READ isAvatar WRITE setIsAvatar)
    Q_PROPERTY(bool isPlayingAvatarSound READ isPlayingAvatarSound)
    Q_PROPERTY(bool isListeningToAudioStream READ isListeningToAudioStream WRITE setIsListeningToAudioStream)
    Q_PROPERTY(bool isNoiseGateEnabled READ isNoiseGateEnabled WRITE setIsNoiseGateEnabled)
    Q_PROPERTY(float lastReceivedAudioLoudness READ getLastReceivedAudioLoudness)
    Q_PROPERTY(QUuid sessionUUID READ getSessionUUID)

public:
    AgentScriptingInterface(Agent* agent);

    bool isPlayingAvatarSound() const { return _agent->isPlayingAvatarSound(); }

    bool isListeningToAudioStream() const { return _agent->isListeningToAudioStream(); }
    void setIsListeningToAudioStream(bool isListeningToAudioStream) const { _agent->setIsListeningToAudioStream(isListeningToAudioStream); }

    bool isNoiseGateEnabled() const { return _agent->isNoiseGateEnabled(); }
    void setIsNoiseGateEnabled(bool isNoiseGateEnabled) const { _agent->setIsNoiseGateEnabled(isNoiseGateEnabled); }

    float getLastReceivedAudioLoudness() const { return _agent->getLastReceivedAudioLoudness(); }
    QUuid getSessionUUID() const { return _agent->getSessionUUID(); }

public slots:
    /**jsdoc
     * @function Agent.setIsAvatar
     * @param {boolean} isAvatar
     */
    void setIsAvatar(bool isAvatar) const { _agent->setIsAvatar(isAvatar); }

    /**jsdoc
     * @function Agent.isAvatar
     * @returns {boolean}
     */
    bool isAvatar() const { return _agent->isAvatar(); }

    /**jsdoc
     * @function Agent.playAvatarSound
     * @param {object} avatarSound
     */
    void playAvatarSound(SharedSoundPointer avatarSound) const { _agent->playAvatarSound(avatarSound); }

private:
    Agent* _agent;

};


#endif // hifi_AgentScriptingInterface_h
