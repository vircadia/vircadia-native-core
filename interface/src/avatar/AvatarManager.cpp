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

AvatarManager::AvatarManager(QObject* parent) :
    _lookAtTargetAvatar(),
    _lookAtOtherPosition(),
    _lookAtIndicatorScale(1.0f),
    _avatarHash(),
    _avatarFades()
{
    
}

void AvatarManager::updateLookAtTargetAvatar(const glm::vec3& mouseRayOrigin, const glm::vec3& mouseRayDirection,
                                             glm::vec3 &eyePosition) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateLookatTargetAvatar()");
    
    if (!Application::getInstance()->isMousePressed()) {
        foreach (const AvatarSharedPointer& avatar, _avatarHash) {
            float distance;
            
            if (avatar->findRayIntersection(mouseRayOrigin, mouseRayDirection, distance)) {
                // rescale to compensate for head embiggening
                eyePosition = (avatar->getHead().calculateAverageEyePosition() - avatar->getHead().getScalePivot()) *
                (avatar->getScale() / avatar->getHead().getScale()) + avatar->getHead().getScalePivot();
                
                _lookAtIndicatorScale = avatar->getHead().getScale();
                _lookAtOtherPosition = avatar->getHead().getPosition();
                
                _lookAtTargetAvatar = avatar;
                
                // found the look at target avatar, return
                return;
            }
        }
        
        _lookAtTargetAvatar.clear();
    }
}

void AvatarManager::updateAvatars(float deltaTime, const glm::vec3& mouseRayOrigin, const glm::vec3& mouseRayDirection) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateAvatars()");
    
    // simulate avatars
    foreach (const AvatarSharedPointer& avatar, _avatarHash) {
        avatar->simulate(deltaTime, NULL);
        avatar->setMouseRay(mouseRayOrigin, mouseRayDirection);
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
    
    if (!selfAvatarOnly) {
        
        //  Render avatars of other nodes
        foreach (const AvatarSharedPointer& avatar, _avatarHash) {
            if (!avatar->isInitialized()) {
                avatar->init();
            }
            avatar->render(false);
            avatar->setDisplayingLookatVectors(Menu::getInstance()->isOptionChecked(MenuOption::LookAtVectors));
        }
        
        renderAvatarFades();
    }
    
    // Render my own Avatar
    Avatar* myAvatar = Application::getInstance()->getAvatar();
    myAvatar->render(forceRenderHead);
    myAvatar->setDisplayingLookatVectors(Menu::getInstance()->isOptionChecked(MenuOption::LookAtVectors));
}

void AvatarManager::simulateAvatarFades(float deltaTime) {
    QVector<AvatarSharedPointer>::iterator fadingAvatar = _avatarFades.begin();
    
    while (fadingAvatar != _avatarFades.end()) {
        const float SHRINK_RATE = 0.9f;
        
        fadingAvatar->data()->setTargetScale(fadingAvatar->data()->getScale() * SHRINK_RATE);
        
        const float MIN_FADE_SCALE = 0.001f;
        
        if (fadingAvatar->data()->getTargetScale() < MIN_FADE_SCALE) {
            fadingAvatar = _avatarFades.erase(fadingAvatar);
        } else {
            fadingAvatar->data()->simulate(deltaTime, NULL);
        }
    }
}

void AvatarManager::renderAvatarFades() {
    // render avatar fades
    Glower glower;
    
    foreach(const AvatarSharedPointer& fadingAvatar, _avatarFades) {
        fadingAvatar->render(false);
    }
}

void AvatarManager::processDataServerResponse(const QString& userString, const QStringList& keyList,
                                              const QStringList &valueList) {
    for (int i = 0; i < keyList.size(); i++) {
        if (valueList[i] != " ") {
            if (keyList[i] == DataServerKey::FaceMeshURL || keyList[i] == DataServerKey::SkeletonURL) {
                // mesh URL for a UUID, find avatar in our list
                AvatarSharedPointer matchingAvatar = _avatarHash.value(QUuid(userString));
                if (matchingAvatar) {
                    if (keyList[i] == DataServerKey::FaceMeshURL) {
                        qDebug() << "Changing mesh to" << valueList[i] << "for avatar with UUID"
                            << uuidStringWithoutCurlyBraces(QUuid(userString));
                        
                        QMetaObject::invokeMethod(&matchingAvatar->getHead().getFaceModel(),
                                                  "setURL", Q_ARG(QUrl, QUrl(valueList[i])));
                    } else if (keyList[i] == DataServerKey::SkeletonURL) {
                        qDebug() << "Changing skeleton to" << valueList[i] << "for avatar with UUID"
                            << uuidStringWithoutCurlyBraces(QString(userString));
                        
                        QMetaObject::invokeMethod(&matchingAvatar->getSkeletonModel(),
                                                  "setURL", Q_ARG(QUrl, QUrl(valueList[i])));
                    }
                }
            }
        }
    }
}

void AvatarManager::processAvatarMixerDatagram(const QByteArray& datagram) {
    unsigned char packetData[MAX_PACKET_SIZE];
    memcpy(packetData, datagram.data(), datagram.size());
    
    int numBytesPacketHeader = numBytesForPacketHeader(packetData);
    
    int bytesRead = numBytesPacketHeader;
    
    unsigned char avatarData[MAX_PACKET_SIZE];
    int numBytesDummyPacketHeader = populateTypeAndVersion(avatarData, PACKET_TYPE_HEAD_DATA);

    while (bytesRead < datagram.size()) {
        QUuid nodeUUID = QUuid::fromRfc4122(datagram.mid(bytesRead, NUM_BYTES_RFC4122_UUID));
        
        AvatarSharedPointer matchingAvatar = _avatarHash.value(nodeUUID);
        
        if (!matchingAvatar) {
            // construct a new Avatar for this node
            matchingAvatar = AvatarSharedPointer(new Avatar());
            
            // insert the new avatar into our hash
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
    AvatarSharedPointer removedAvatar = _avatarHash.take(nodeUUID);
    
    if (removedAvatar) {
        // add this avatar to our vector of fades
        _avatarFades.push_back(removedAvatar);
    }
}

void AvatarManager::clearHash() {
    // clear the AvatarManager hash - typically happens on the removal of the avatar-mixer
    _avatarHash.clear();
}