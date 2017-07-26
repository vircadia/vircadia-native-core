//
//  UsersScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-07-11.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "UsersScriptingInterface.h"

#include <NodeList.h>

UsersScriptingInterface::UsersScriptingInterface() {
    // emit a signal when kick permissions have changed
    auto nodeList = DependencyManager::get<NodeList>();
    connect(nodeList.data(), &LimitedNodeList::canKickChanged, this, &UsersScriptingInterface::canKickChanged);
    connect(nodeList.data(), &NodeList::ignoreRadiusEnabledChanged, this, &UsersScriptingInterface::ignoreRadiusEnabledChanged);
    connect(nodeList.data(), &NodeList::usernameFromIDReply, this, &UsersScriptingInterface::usernameFromIDReply);
}

void UsersScriptingInterface::ignore(const QUuid& nodeID, bool ignoreEnabled) {
    // ask the NodeList to ignore this user (based on the session ID of their node)
    DependencyManager::get<NodeList>()->ignoreNodeBySessionID(nodeID, ignoreEnabled);
}

bool UsersScriptingInterface::getIgnoreStatus(const QUuid& nodeID) {
    // ask the NodeList for the Ignore status associated with the given session ID
    return DependencyManager::get<NodeList>()->isIgnoringNode(nodeID);
}

void UsersScriptingInterface::personalMute(const QUuid& nodeID, bool muteEnabled) {
    // ask the NodeList to mute the user with the given session ID
	// "Personal Mute" only applies one way and is not global
    DependencyManager::get<NodeList>()->personalMuteNodeBySessionID(nodeID, muteEnabled);
}

bool UsersScriptingInterface::getPersonalMuteStatus(const QUuid& nodeID) {
    // ask the NodeList for the Personal Mute status associated with the given session ID
    return DependencyManager::get<NodeList>()->isPersonalMutingNode(nodeID);
}

void UsersScriptingInterface::setAvatarGain(const QUuid& nodeID, float gain) {
    // ask the NodeList to set the gain of the specified avatar
    DependencyManager::get<NodeList>()->setAvatarGain(nodeID, gain);
}

float UsersScriptingInterface::getAvatarGain(const QUuid& nodeID) {
    return DependencyManager::get<NodeList>()->getAvatarGain(nodeID);
}

void UsersScriptingInterface::kick(const QUuid& nodeID) {
    // ask the NodeList to kick the user with the given session ID
    DependencyManager::get<NodeList>()->kickNodeBySessionID(nodeID);
}

void UsersScriptingInterface::mute(const QUuid& nodeID) {
    // ask the NodeList to mute the user with the given session ID
    DependencyManager::get<NodeList>()->muteNodeBySessionID(nodeID);
}

void UsersScriptingInterface::requestUsernameFromID(const QUuid& nodeID) {
    // ask the Domain Server via the NodeList for the username associated with the given session ID
    DependencyManager::get<NodeList>()->requestUsernameFromSessionID(nodeID);
}

bool UsersScriptingInterface::getCanKick() {
    // ask the NodeList to return our ability to kick
    return DependencyManager::get<NodeList>()->getThisNodeCanKick();
}

void UsersScriptingInterface::toggleIgnoreRadius() {
    DependencyManager::get<NodeList>()->toggleIgnoreRadius();
}

void UsersScriptingInterface::enableIgnoreRadius() {
    DependencyManager::get<NodeList>()->enableIgnoreRadius();
}

void UsersScriptingInterface::disableIgnoreRadius() {
    DependencyManager::get<NodeList>()->disableIgnoreRadius();
}

bool UsersScriptingInterface::getIgnoreRadiusEnabled() {
    return DependencyManager::get<NodeList>()->getIgnoreRadiusEnabled();
}

bool UsersScriptingInterface::getRequestsDomainListData() {
    return DependencyManager::get<NodeList>()->getRequestsDomainListData();
}
void UsersScriptingInterface::setRequestsDomainListData(bool isRequesting) {
    DependencyManager::get<NodeList>()->setRequestsDomainListData(isRequesting);
}
