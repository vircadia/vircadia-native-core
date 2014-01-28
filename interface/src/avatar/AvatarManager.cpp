//
//  AvatarManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <PerfStat.h>
#include <UUID.h>

#include "Application.h"
#include "Avatar.h"
#include "Menu.h"
#include "MyAvatar.h"

#include "AvatarManager.h"

// We add _myAvatar into the hash with all the other AvatarData, and we use the default NULL QUid as the key.
const QUuid MY_AVATAR_KEY;  // NULL key

AvatarManager::AvatarManager(QObject* parent) :
    _lookAtTargetAvatar(),
    _lookAtOtherPosition(),
    _lookAtIndicatorScale(1.0f),
    _avatarFades(),
    _myAvatar(NULL)
{
    // register a meta type for the weak pointer we'll use for the owning avatar mixer for each avatar
    qRegisterMetaType<QWeakPointer<Node> >("NodeWeakPointer");
}

void AvatarManager::setMyAvatar(MyAvatar* myAvatar) {
    if (!_myAvatar) {
        // can only ever set this once
        _myAvatar = myAvatar;
        // add _myAvatar to the list
        AvatarSharedPointer myPointer = AvatarSharedPointer(_myAvatar);
        _avatarHash.insert(MY_AVATAR_KEY, myPointer);
    }
}

void AvatarManager::updateLookAtTargetAvatar(glm::vec3 &eyePosition) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateLookatTargetAvatar()");
    
    Application* applicationInstance = Application::getInstance();
    
    if (!applicationInstance->isMousePressed()) {
        glm::vec3 mouseOrigin = applicationInstance->getMouseRayOrigin();
        glm::vec3 mouseDirection = applicationInstance->getMouseRayDirection();

        foreach (const AvatarSharedPointer& avatarPointer, _avatarHash) {
            Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
            if (avatar != static_cast<Avatar*>(_myAvatar)) {
                float distance;
                if (avatar->findRayIntersection(mouseOrigin, mouseDirection, distance)) {
                    // rescale to compensate for head embiggening
                    eyePosition = (avatar->getHead().calculateAverageEyePosition() - avatar->getHead().getScalePivot()) *
                        (avatar->getScale() / avatar->getHead().getScale()) + avatar->getHead().getScalePivot();
                    
                    _lookAtIndicatorScale = avatar->getHead().getScale();
                    _lookAtOtherPosition = avatar->getHead().getPosition();
                    
                    _lookAtTargetAvatar = avatarPointer;
                    
                    // found the look at target avatar, return
                    return;
                }
            }
        }
        
        _lookAtTargetAvatar.clear();
    }
}

void AvatarManager::updateAvatars(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateAvatars()");
    
    Application* applicationInstance = Application::getInstance();
    glm::vec3 mouseOrigin = applicationInstance->getMouseRayOrigin();
    glm::vec3 mouseDirection = applicationInstance->getMouseRayDirection();

    // simulate avatars
    AvatarHash::iterator avatarIterator = _avatarHash.begin();
    while (avatarIterator != _avatarHash.end()) {
        Avatar* avatar = static_cast<Avatar*>(avatarIterator.value().data());
        if (avatar == static_cast<Avatar*>(_myAvatar)) {
            // for now skip updates to _myAvatar because it is done explicitly in Application
            // TODO: update _myAvatar in this context
            ++avatarIterator;
            continue;
        }
        if (avatar->getOwningAvatarMixer()) {
            // this avatar's mixer is still around, go ahead and simulate it
            avatar->simulate(deltaTime, NULL);
            avatar->setMouseRay(mouseOrigin, mouseDirection);
            ++avatarIterator;
        } else {
            // the mixer that owned this avatar is gone, give it to the vector of fades and kill it
            avatarIterator = erase(avatarIterator);
        }
    }
    
    // simulate avatar fades
    simulateAvatarFades(deltaTime);
}

void AvatarManager::renderAvatars(bool forceRenderHead, bool selfAvatarOnly) {
    if (!Menu::getInstance()->isOptionChecked(MenuOption::Avatars)) {
        return;
    }
    PerformanceWarning warn(Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings),
                            "Application::renderAvatars()");
    bool renderLookAtVectors = Menu::getInstance()->isOptionChecked(MenuOption::LookAtVectors);
    
    if (!selfAvatarOnly) {
        //  Render avatars of other nodes
        foreach (const AvatarSharedPointer& avatarPointer, _avatarHash) {
            Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
            if (!avatar->isInitialized()) {
                avatar->init();
            }
            if (avatar == static_cast<Avatar*>(_myAvatar)) {
                avatar->render(forceRenderHead);
            } else {
                avatar->render(false);
            }
            avatar->setDisplayingLookatVectors(renderLookAtVectors);
        }
        renderAvatarFades();
    } else if (_myAvatar) {
        // Render my own Avatar
        _myAvatar->render(forceRenderHead);
        _myAvatar->setDisplayingLookatVectors(renderLookAtVectors);
    }
}

void AvatarManager::simulateAvatarFades(float deltaTime) {
    QVector<AvatarSharedPointer>::iterator fadingIterator = _avatarFades.begin();
    
    const float SHRINK_RATE = 0.9f;
    const float MIN_FADE_SCALE = 0.001f;

    while (fadingIterator != _avatarFades.end()) {
        Avatar* avatar = static_cast<Avatar*>(fadingIterator->data());
        avatar->setTargetScale(avatar->getScale() * SHRINK_RATE);
        if (avatar->getTargetScale() < MIN_FADE_SCALE) {
            fadingIterator = _avatarFades.erase(fadingIterator);
        } else {
            avatar->simulate(deltaTime, NULL);
            ++fadingIterator;
        }
    }
}

void AvatarManager::renderAvatarFades() {
    // render avatar fades
    Glower glower;
    
    foreach(const AvatarSharedPointer& fadingAvatar, _avatarFades) {
        Avatar* avatar = static_cast<Avatar*>(fadingAvatar.data());
        avatar->render(false);
    }
}

void AvatarManager::processDataServerResponse(const QString& userString, const QStringList& keyList,
                                              const QStringList &valueList) {
    QUuid avatarKey = QUuid(userString);
    if (avatarKey == MY_AVATAR_KEY) {
        // ignore updates to our own mesh
        return;
    }
    for (int i = 0; i < keyList.size(); i++) {
        if (valueList[i] != " ") {
            if (keyList[i] == DataServerKey::FaceMeshURL || keyList[i] == DataServerKey::SkeletonURL) {
                // mesh URL for a UUID, find avatar in our list
                AvatarSharedPointer matchingAvatar = _avatarHash.value(avatarKey);
                if (matchingAvatar) {
                    Avatar* avatar = static_cast<Avatar*>(matchingAvatar.data());
                    if (keyList[i] == DataServerKey::FaceMeshURL) {
                        qDebug() << "Changing mesh to" << valueList[i] << "for avatar with UUID"
                            << uuidStringWithoutCurlyBraces(avatarKey);
                        
                        QMetaObject::invokeMethod(&(avatar->getHead().getFaceModel()),
                                                  "setURL", Q_ARG(QUrl, QUrl(valueList[i])));
                    } else if (keyList[i] == DataServerKey::SkeletonURL) {
                        qDebug() << "Changing skeleton to" << valueList[i] << "for avatar with UUID"
                            << uuidStringWithoutCurlyBraces(avatarKey.toString());
                        
                        QMetaObject::invokeMethod(&(avatar->getSkeletonModel()),
                                                  "setURL", Q_ARG(QUrl, QUrl(valueList[i])));
                    }
                }
            }
        }
    }
}

void AvatarManager::processAvatarMixerDatagram(const QByteArray& datagram, const QWeakPointer<Node>& mixerWeakPointer) {
    unsigned char packetData[MAX_PACKET_SIZE];
    memcpy(packetData, datagram.data(), datagram.size());
    
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    
    int bytesRead = numBytesPacketHeader;
    
    unsigned char avatarData[MAX_PACKET_SIZE];
    int numBytesDummyPacketHeader = populateTypeAndVersion(avatarData, PACKET_TYPE_HEAD_DATA);

    // enumerate over all of the avatars in this packet
    // only add them if mixerWeakPointer points to something (meaning that mixer is still around)
    while (bytesRead < datagram.size() && mixerWeakPointer.data()) {
        QUuid nodeUUID = QUuid::fromRfc4122(datagram.mid(bytesRead, NUM_BYTES_RFC4122_UUID));
        // TODO: skip the data if nodeUUID is same as MY_AVATAR_KEY
        
        AvatarSharedPointer matchingAvatar = _avatarHash.value(nodeUUID);
        
        if (!matchingAvatar) {
            // construct a new Avatar for this node
            Avatar* avatar =  new Avatar();
            avatar->setOwningAvatarMixer(mixerWeakPointer);
            
            // insert the new avatar into our hash
            matchingAvatar = AvatarSharedPointer(avatar);
            _avatarHash.insert(nodeUUID, matchingAvatar);
            
            // new UUID requires mesh and skeleton request to data-server
            DataServerClient::getValuesForKeysAndUUID(QStringList() << DataServerKey::FaceMeshURL << DataServerKey::SkeletonURL,
                                                      nodeUUID, this);
            
            qDebug() << "Adding avatar with UUID" << nodeUUID << "to AvatarManager hash.";
        }
        
        // copy the rest of the packet to the avatarData holder so we can read the next Avatar from there
        memcpy(avatarData + numBytesDummyPacketHeader, packetData + bytesRead, datagram.size() - bytesRead);
        
        // have the matching (or new) avatar parse the data from the packet
        bytesRead += matchingAvatar->parseData(avatarData, datagram.size() - bytesRead);
    }
}

void AvatarManager::processKillAvatar(const QByteArray& datagram) {
    // read the node id
    QUuid nodeUUID = QUuid::fromRfc4122(datagram.mid(numBytesForPacketHeader(reinterpret_cast<const unsigned char*>
                                                                             (datagram.data())),
                                                     NUM_BYTES_RFC4122_UUID));
    
    // remove the avatar with that UUID from our hash, if it exists
    AvatarHash::iterator matchedAvatar = _avatarHash.find(nodeUUID);
    if (matchedAvatar != _avatarHash.end()) {
        erase(matchedAvatar);
    }
}

AvatarHash::iterator AvatarManager::erase(const AvatarHash::iterator& iterator) {
    if (iterator.key() != MY_AVATAR_KEY) {
        qDebug() << "Removing Avatar with UUID" << iterator.key() << "from AvatarManager hash.";
        _avatarFades.push_back(iterator.value());
        return AvatarHashMap::erase(iterator);
    } else {
        // never remove _myAvatar from the list
        AvatarHash::iterator returnIterator = iterator;
        return ++returnIterator;
    }
}

void AvatarManager::clearHash() {
    // clear the AvatarManager hash - typically happens on the removal of the avatar-mixer
    AvatarHash::iterator removeAvatar =  _avatarHash.begin();
    while (removeAvatar != _avatarHash.end()) {
        removeAvatar = erase(removeAvatar);
    }
}
