//
//  AvatarManager.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
#include <string>
#include <PerfStat.h>
#include <UUID.h>
#include <glm/gtx/string_cast.hpp>

#include "Application.h"
#include "Avatar.h"
#include "Menu.h"
#include "MyAvatar.h"

#include "AvatarManager.h"

// We add _myAvatar into the hash with all the other AvatarData, and we use the default NULL QUid as the key.
const QUuid MY_AVATAR_KEY;  // NULL key

AvatarManager::AvatarManager(QObject* parent) :
    _avatarFades() {
    // register a meta type for the weak pointer we'll use for the owning avatar mixer for each avatar
    qRegisterMetaType<QWeakPointer<Node> >("NodeWeakPointer");
    _myAvatar = QSharedPointer<MyAvatar>(new MyAvatar());
}

void AvatarManager::init() {
    _myAvatar->init();
    _myAvatar->setPosition(START_LOCATION);
    _myAvatar->setDisplayingLookatVectors(false);
    _avatarHash.insert(MY_AVATAR_KEY, _myAvatar);
}

void AvatarManager::updateOtherAvatars(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateAvatars()");

    Application* applicationInstance = Application::getInstance();
    glm::vec3 mouseOrigin = applicationInstance->getMouseRayOrigin();
    glm::vec3 mouseDirection = applicationInstance->getMouseRayDirection();

    // simulate avatars
    AvatarHash::iterator avatarIterator = _avatarHash.begin();
    while (avatarIterator != _avatarHash.end()) {
        Avatar* avatar = static_cast<Avatar*>(avatarIterator.value().data());
        if (avatar == static_cast<Avatar*>(_myAvatar.data())) {
            // DO NOT update _myAvatar!  Its update has already been done earlier in the main loop.
            //updateMyAvatar(deltaTime);
            ++avatarIterator;
            continue;
        }
        if (avatar->getOwningAvatarMixer()) {
            // this avatar's mixer is still around, go ahead and simulate it
            avatar->simulate(deltaTime);
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
    
    // -------------------------------------------------------------------------------------
    // Josecpujol(begin): test code, as I dont seem to get any avatar. Render some text here
    // -------------------------------------------------------------------------------------
    glm::dvec3 textPosition(100.0, 50.0, 100.0);
    std::string text("my string");

    // Draw a fake avatar
    glPushMatrix();
    glColor3f(1.0,0,0);
    double radius = 10.0;
    glTranslated(textPosition.x, textPosition.y - 2 * radius, textPosition.z);
    glutSolidSphere(radius, 10, 10);
    glPopMatrix();

    // Draw a mark where the text should be
    glPushMatrix();
    glColor3f(1.0,1.0,0);
    glTranslated(textPosition.x, textPosition.y, textPosition.z); 
    glutSolidSphere(1.0, 10, 10);
    glPopMatrix();
        
    glPushMatrix();
    glm::dmat4 modelViewMatrix2;
    glGetDoublev(GL_MODELVIEW_MATRIX, (GLdouble*)&modelViewMatrix2);
    glTranslated(textPosition.x, textPosition.y, textPosition.z); 
    // Extract rotation matrix from the modelview matrix
    glm::dmat4 modelViewMatrix;
    glGetDoublev(GL_MODELVIEW_MATRIX, (GLdouble*)&modelViewMatrix);
  
    // Delete rotation info
    modelViewMatrix[0][0] = modelViewMatrix[1][1] = modelViewMatrix[2][2] = 1.0;
    modelViewMatrix[0][1] = modelViewMatrix[0][2] = 0.0;
    modelViewMatrix[1][0] = modelViewMatrix[1][2] = 0.0;
    modelViewMatrix[2][0] = modelViewMatrix[2][1] = 0.0;

    glLoadMatrixd((GLdouble*)&modelViewMatrix);  // Override current matrix with our own
    glScalef(1.0, -1.0, 1.0);  // TextRenderer::draw paints the text upside down. This fixes that

    // We need to compute the scale factor such as the text remains with fixed size respect to window coordinates
    // We project y = 0 and y = 1  and check the difference in projection coordinates
    GLdouble projectionMatrix[16];
    GLint viewportMatrix[4];
    GLdouble result0[3];
    GLdouble result1[3];

    glm::dvec3 upVector(modelViewMatrix2[1]);
    glGetDoublev(GL_PROJECTION_MATRIX, (GLdouble*)&projectionMatrix);
    glGetIntegerv(GL_VIEWPORT, viewportMatrix);

    glm::dvec3 testPoint0 = textPosition;
    glm::dvec3 testPoint1 = textPosition +  upVector;
    
    bool success;
    success = gluProject(testPoint0.x, testPoint0.y, testPoint0.z,
        (GLdouble*)&modelViewMatrix2, projectionMatrix, viewportMatrix, 
        &result0[0], &result0[1], &result0[2]);
    success = success && 
        gluProject(testPoint1.x, testPoint1.y, testPoint1.z,
        (GLdouble*)&modelViewMatrix2, projectionMatrix, viewportMatrix, 
        &result1[0], &result1[1], &result1[2]);

    if (success) {
        double textWindowHeight = abs(result1[1] - result0[1]);
        float textScale = 1.0;
        float scaleFactor = textScale / textWindowHeight;

        glScalef(scaleFactor, scaleFactor, 1.0);

        glColor3f(0.93, 0.93, 0.93);
        
        // TextRenderer, based on QT opengl text rendering functions
        TextRenderer* textRenderer = new TextRenderer(SANS_FONT_FAMILY, 12, -1, false, TextRenderer::NO_EFFECT);
        int width = 0;
        for (std::string::iterator it = text.begin(); it != text.end(); it++) {
            width += (textRenderer->computeWidth(*it));
        }
        textRenderer->draw(-width/2.0, 0, text.c_str());   
        delete textRenderer;
    }
   
    glPopMatrix();

    // -------------------------------------------------------------------------------------
    // josecpujol(end)
    // -------------------------------------------------------------------------------------



    if (!selfAvatarOnly) {
        foreach (const AvatarSharedPointer& avatarPointer, _avatarHash) {
            Avatar* avatar = static_cast<Avatar*>(avatarPointer.data());
            if (!avatar->isInitialized()) {
                avatar->init();
            }
            if (avatar == static_cast<Avatar*>(_myAvatar.data())) {
                avatar->render(forceRenderHead);
            } else {
                avatar->render(true);
            }
            avatar->setDisplayingLookatVectors(renderLookAtVectors);
        }
        renderAvatarFades();
    } else {
        // just render myAvatar
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
            avatar->simulate(deltaTime);
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

void AvatarManager::processAvatarMixerDatagram(const QByteArray& datagram, const QWeakPointer<Node>& mixerWeakPointer) {
    switch (packetTypeForPacket(datagram)) {
        case PacketTypeBulkAvatarData:
            processAvatarDataPacket(datagram, mixerWeakPointer);
            break;
        case PacketTypeAvatarIdentity:
            processAvatarIdentityPacket(datagram);
            break;
        case PacketTypeKillAvatar:
            processKillAvatar(datagram);
            break;
        default:
            break;
    }
}

void AvatarManager::processAvatarDataPacket(const QByteArray &datagram, const QWeakPointer<Node> &mixerWeakPointer) {
    int bytesRead = numBytesForPacketHeader(datagram);
    
    QByteArray dummyAvatarByteArray = byteArrayWithPopluatedHeader(PacketTypeAvatarData);
    int numDummyHeaderBytes = dummyAvatarByteArray.size();
    int numDummyHeaderBytesWithoutUUID = numDummyHeaderBytes - NUM_BYTES_RFC4122_UUID;
    
    // enumerate over all of the avatars in this packet
    // only add them if mixerWeakPointer points to something (meaning that mixer is still around)
    while (bytesRead < datagram.size() && mixerWeakPointer.data()) {
        QUuid nodeUUID = QUuid::fromRfc4122(datagram.mid(bytesRead, NUM_BYTES_RFC4122_UUID));
        
        AvatarSharedPointer matchingAvatar = _avatarHash.value(nodeUUID);
        
        if (!matchingAvatar) {
            // construct a new Avatar for this node
            Avatar* avatar =  new Avatar();
            avatar->setOwningAvatarMixer(mixerWeakPointer);
            
            // insert the new avatar into our hash
            matchingAvatar = AvatarSharedPointer(avatar);
            _avatarHash.insert(nodeUUID, matchingAvatar);
            
            qDebug() << "Adding avatar with UUID" << nodeUUID << "to AvatarManager hash.";
        }
        
        // copy the rest of the packet to the avatarData holder so we can read the next Avatar from there
        dummyAvatarByteArray.resize(numDummyHeaderBytesWithoutUUID);
        
        // make this Avatar's UUID the UUID in the packet and tack the remaining data onto the end
        dummyAvatarByteArray.append(datagram.mid(bytesRead));
        
        // have the matching (or new) avatar parse the data from the packet
        bytesRead += matchingAvatar->parseData(dummyAvatarByteArray) - numDummyHeaderBytesWithoutUUID;
    }

}

void AvatarManager::processAvatarIdentityPacket(const QByteArray &packet) {
    // setup a data stream to parse the packet
    QDataStream identityStream(packet);
    identityStream.skipRawData(numBytesForPacketHeader(packet));
    
    QUuid nodeUUID;
    
    while (!identityStream.atEnd()) {
        
        QUrl faceMeshURL, skeletonURL;
        identityStream >> nodeUUID >> faceMeshURL >> skeletonURL;
        
        // mesh URL for a UUID, find avatar in our list
        AvatarSharedPointer matchingAvatar = _avatarHash.value(nodeUUID);
        if (matchingAvatar) {
            Avatar* avatar = static_cast<Avatar*>(matchingAvatar.data());
            
            if (avatar->getFaceModelURL() != faceMeshURL) {
                avatar->setFaceModelURL(faceMeshURL);
            }
            
            if (avatar->getSkeletonModelURL() != skeletonURL) {
                avatar->setSkeletonModelURL(skeletonURL);
            }
        }
    }
}

void AvatarManager::processKillAvatar(const QByteArray& datagram) {
    // read the node id
    QUuid nodeUUID = QUuid::fromRfc4122(datagram.mid(numBytesForPacketHeader(datagram), NUM_BYTES_RFC4122_UUID));
    
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

void AvatarManager::clearOtherAvatars() {
    // clear any avatars that came from an avatar-mixer
    AvatarHash::iterator removeAvatar =  _avatarHash.begin();
    while (removeAvatar != _avatarHash.end()) {
        removeAvatar = erase(removeAvatar);
    }
    _myAvatar->clearLookAtTargetAvatar();
}
