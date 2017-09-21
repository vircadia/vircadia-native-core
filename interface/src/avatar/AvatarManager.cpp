//
//  AvatarManager.cpp
//  interface/src/avatar
//
//  Created by Stephen Birarda on 1/23/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <string>

#include <QScriptEngine>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

#include <glm/gtx/string_cast.hpp>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif


#include <shared/QtHelpers.h>
#include <AvatarData.h>
#include <PerfStat.h>
#include <RegisteredMetaTypes.h>
#include <Rig.h>
#include <SettingHandle.h>
#include <UsersScriptingInterface.h>
#include <UUID.h>
#include <avatars-renderer/OtherAvatar.h>

#include "Application.h"
#include "AvatarManager.h"
#include "InterfaceLogging.h"
#include "Menu.h"
#include "MyAvatar.h"
#include "SceneScriptingInterface.h"

// 50 times per second - target is 45hz, but this helps account for any small deviations
// in the update loop - this also results in ~30hz when in desktop mode which is essentially
// what we want
const int CLIENT_TO_AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 50;
static const quint64 MIN_TIME_BETWEEN_MY_AVATAR_DATA_SENDS = USECS_PER_SECOND / CLIENT_TO_AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND;

// We add _myAvatar into the hash with all the other AvatarData, and we use the default NULL QUid as the key.
const QUuid MY_AVATAR_KEY;  // NULL key

AvatarManager::AvatarManager(QObject* parent) :
    _avatarsToFade(),
    _myAvatar(std::make_shared<MyAvatar>(qApp->thread()))
{
    // register a meta type for the weak pointer we'll use for the owning avatar mixer for each avatar
    qRegisterMetaType<QWeakPointer<Node> >("NodeWeakPointer");

    auto nodeList = DependencyManager::get<NodeList>();
    auto& packetReceiver = nodeList->getPacketReceiver();
    packetReceiver.registerListener(PacketType::BulkAvatarData, this, "processAvatarDataPacket");
    packetReceiver.registerListener(PacketType::KillAvatar, this, "processKillAvatar");
    packetReceiver.registerListener(PacketType::AvatarIdentity, this, "processAvatarIdentityPacket");

    // when we hear that the user has ignored an avatar by session UUID
    // immediately remove that avatar instead of waiting for the absence of packets from avatar mixer
    connect(nodeList.data(), &NodeList::ignoredNode, this, [=](const QUuid& nodeID, bool enabled) {
        if (enabled) {
            removeAvatar(nodeID, KillAvatarReason::AvatarIgnored);
        }
    });
}

AvatarManager::~AvatarManager() {
    assert(_motionStates.empty());
}

void AvatarManager::init() {
    _myAvatar->init();
    {
        QWriteLocker locker(&_hashLock);
        _avatarHash.insert(MY_AVATAR_KEY, _myAvatar);
    }

    _shouldRender = DependencyManager::get<SceneScriptingInterface>()->shouldRenderAvatars();
    connect(DependencyManager::get<SceneScriptingInterface>().data(), &SceneScriptingInterface::shouldRenderAvatarsChanged,
            this, &AvatarManager::updateAvatarRenderStatus, Qt::QueuedConnection);

    if (_shouldRender) {
        const render::ScenePointer& scene = qApp->getMain3DScene();
        render::Transaction transaction;
        _myAvatar->addToScene(_myAvatar, scene, transaction);
        scene->enqueueTransaction(transaction);
    }
}

void AvatarManager::updateMyAvatar(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "AvatarManager::updateMyAvatar()");

    _myAvatar->update(deltaTime);

    quint64 now = usecTimestampNow();
    quint64 dt = now - _lastSendAvatarDataTime;

    if (dt > MIN_TIME_BETWEEN_MY_AVATAR_DATA_SENDS) {
        // send head/hand data to the avatar mixer and voxel server
        PerformanceTimer perfTimer("send");
        _myAvatar->sendAvatarDataPacket();
        _lastSendAvatarDataTime = now;
        _myAvatarSendRate.increment();
    }
}


Q_LOGGING_CATEGORY(trace_simulation_avatar, "trace.simulation.avatar");

float AvatarManager::getAvatarDataRate(const QUuid& sessionID, const QString& rateName) const {
    auto avatar = getAvatarBySessionID(sessionID);
    return avatar ? avatar->getDataRate(rateName) : 0.0f;
}

float AvatarManager::getAvatarUpdateRate(const QUuid& sessionID, const QString& rateName) const {
    auto avatar = getAvatarBySessionID(sessionID);
    return avatar ? avatar->getUpdateRate(rateName) : 0.0f;
}

float AvatarManager::getAvatarSimulationRate(const QUuid& sessionID, const QString& rateName) const {
    auto avatar = std::static_pointer_cast<Avatar>(getAvatarBySessionID(sessionID));
    return avatar ? avatar->getSimulationRate(rateName) : 0.0f;
}

void AvatarManager::updateOtherAvatars(float deltaTime) {
    // lock the hash for read to check the size
    QReadLocker lock(&_hashLock);
    if (_avatarHash.size() < 2 && _avatarsToFade.isEmpty()) {
        return;
    }
    lock.unlock();

    PerformanceTimer perfTimer("otherAvatars");

    auto avatarMap = getHashCopy();
    QList<AvatarSharedPointer> avatarList = avatarMap.values();
    ViewFrustum cameraView;
    qApp->copyDisplayViewFrustum(cameraView);

    std::priority_queue<AvatarPriority> sortedAvatars;
    AvatarData::sortAvatars(avatarList, cameraView, sortedAvatars,

        [](AvatarSharedPointer avatar)->uint64_t{
            return std::static_pointer_cast<Avatar>(avatar)->getLastRenderUpdateTime();
        },

        [](AvatarSharedPointer avatar)->float{
            return std::static_pointer_cast<Avatar>(avatar)->getBoundingRadius();
        },

        [this](AvatarSharedPointer avatar)->bool{
            const auto& castedAvatar = std::static_pointer_cast<Avatar>(avatar);
            if (castedAvatar == _myAvatar || !castedAvatar->isInitialized()) {
                // DO NOT update _myAvatar!  Its update has already been done earlier in the main loop.
                // DO NOT update or fade out uninitialized Avatars
                return true; // ignore it
            }
            return false;
        });

    uint64_t startTime = usecTimestampNow();
    const uint64_t UPDATE_BUDGET = 2000; // usec
    uint64_t updateExpiry = startTime + UPDATE_BUDGET;
    int numAvatarsUpdated = 0;
    int numAVatarsNotUpdated = 0;

    render::Transaction transaction;
    while (!sortedAvatars.empty()) {
        const AvatarPriority& sortData = sortedAvatars.top();
        const auto& avatar = std::static_pointer_cast<Avatar>(sortData.avatar);

        // for ALL avatars...
        if (_shouldRender) {
            avatar->ensureInScene(avatar, qApp->getMain3DScene());
        }
        if (!avatar->isInPhysicsSimulation()) {
            ShapeInfo shapeInfo;
            avatar->computeShapeInfo(shapeInfo);
            btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
            if (shape) {
                AvatarMotionState* motionState = new AvatarMotionState(avatar, shape);
                motionState->setMass(avatar->computeMass());
                avatar->setPhysicsCallback([=] (uint32_t flags) { motionState->addDirtyFlags(flags); });
                _motionStates.insert(avatar.get(), motionState);
                _motionStatesToAddToPhysics.insert(motionState);
            }
        }
        avatar->animateScaleChanges(deltaTime);

        const float OUT_OF_VIEW_THRESHOLD = 0.5f * AvatarData::OUT_OF_VIEW_PENALTY;
        uint64_t now = usecTimestampNow();
        if (now < updateExpiry) {
            // we're within budget
            bool inView = sortData.priority > OUT_OF_VIEW_THRESHOLD;
            if (inView && avatar->hasNewJointData()) {
                numAvatarsUpdated++;
            }
            avatar->simulate(deltaTime, inView);
            avatar->updateRenderItem(transaction);
            avatar->setLastRenderUpdateTime(startTime);
        } else {
            // we've spent our full time budget --> bail on the rest of the avatar updates
            // --> more avatars may freeze until their priority trickles up
            // --> some scale or fade animations may glitch
            // --> some avatar velocity measurements may be a little off

            // no time simulate, but we take the time to count how many were tragically missed
            bool inView = sortData.priority > OUT_OF_VIEW_THRESHOLD;
            if (!inView) {
                break;
            }
            if (inView && avatar->hasNewJointData()) {
                numAVatarsNotUpdated++;
            }
            sortedAvatars.pop();
            while (inView && !sortedAvatars.empty()) {
                const AvatarPriority& newSortData = sortedAvatars.top();
                const auto& newAvatar = std::static_pointer_cast<Avatar>(newSortData.avatar);
                inView = newSortData.priority > OUT_OF_VIEW_THRESHOLD;
                if (inView && newAvatar->hasNewJointData()) {
                    numAVatarsNotUpdated++;
                }
                sortedAvatars.pop();
            }
            break;
        }
        sortedAvatars.pop();
    }

    if (_shouldRender) {
        if (!_avatarsToFade.empty()) {
            QReadLocker lock(&_hashLock);
            QVector<AvatarSharedPointer>::iterator itr = _avatarsToFade.begin();
            while (itr != _avatarsToFade.end() && usecTimestampNow() > updateExpiry) {
                auto avatar = std::static_pointer_cast<Avatar>(*itr);
                avatar->animateScaleChanges(deltaTime);
                avatar->simulate(deltaTime, true);
                avatar->updateRenderItem(transaction);
                ++itr;
            }
        }
        qApp->getMain3DScene()->enqueueTransaction(transaction);
    }

    _avatarSimulationTime = (float)(usecTimestampNow() - startTime) / (float)USECS_PER_MSEC;
    _numAvatarsUpdated = numAvatarsUpdated;
    _numAvatarsNotUpdated = numAVatarsNotUpdated;

    simulateAvatarFades(deltaTime);
}

void AvatarManager::postUpdate(float deltaTime, const render::ScenePointer& scene) {
    auto hashCopy = getHashCopy();
    AvatarHash::iterator avatarIterator = hashCopy.begin();
    for (avatarIterator = hashCopy.begin(); avatarIterator != hashCopy.end(); avatarIterator++) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarIterator.value());
        avatar->postUpdate(deltaTime, scene);
    }
}

void AvatarManager::simulateAvatarFades(float deltaTime) {
    if (_avatarsToFade.empty()) {
        return;
    }

    QReadLocker locker(&_hashLock);
    QVector<AvatarSharedPointer>::iterator avatarItr = _avatarsToFade.begin();
    const render::ScenePointer& scene = qApp->getMain3DScene();
    while (avatarItr != _avatarsToFade.end()) {
        auto avatar = std::static_pointer_cast<Avatar>(*avatarItr);
        avatar->updateFadingStatus(scene);
        if (!avatar->isFading()) {
            // fading to zero is such a rare event we push a unique transaction for each
            if (avatar->isInScene()) {
                render::Transaction transaction;
                avatar->removeFromScene(*avatarItr, scene, transaction);
                scene->enqueueTransaction(transaction);
            }
            avatarItr = _avatarsToFade.erase(avatarItr);
        } else {
            const bool inView = true; // HACK
            avatar->simulate(deltaTime, inView);
            ++avatarItr;
        }
    }
}

AvatarSharedPointer AvatarManager::newSharedAvatar() {
    return std::make_shared<OtherAvatar>(qApp->thread());
}

void AvatarManager::handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason) {
    AvatarHashMap::handleRemovedAvatar(removedAvatar, removalReason);

    // remove from physics
    auto avatar = std::static_pointer_cast<Avatar>(removedAvatar);
    avatar->setPhysicsCallback(nullptr);
    AvatarMotionStateMap::iterator itr = _motionStates.find(avatar.get());
    if (itr != _motionStates.end()) {
        AvatarMotionState* motionState = *itr;
        _motionStatesToAddToPhysics.remove(motionState);
        _motionStatesToRemoveFromPhysics.push_back(motionState);
        _motionStates.erase(itr);
    }

    if (removalReason == KillAvatarReason::TheirAvatarEnteredYourBubble) {
        emit DependencyManager::get<UsersScriptingInterface>()->enteredIgnoreRadius();
    } else if (removalReason == KillAvatarReason::AvatarDisconnected) {
        // remove from node sets, if present
        DependencyManager::get<NodeList>()->removeFromIgnoreMuteSets(avatar->getSessionUUID());
        DependencyManager::get<UsersScriptingInterface>()->avatarDisconnected(avatar->getSessionUUID());
        avatar->fadeOut(qApp->getMain3DScene(), removalReason);
    }
    _avatarsToFade.push_back(removedAvatar);
}

void AvatarManager::clearOtherAvatars() {
    // Remove other avatars from the world but don't actually remove them from _avatarHash
    // each will either be removed on timeout or will re-added to the world on receipt of update.
    const render::ScenePointer& scene = qApp->getMain3DScene();
    render::Transaction transaction;

    QReadLocker locker(&_hashLock);
    AvatarHash::iterator avatarIterator =  _avatarHash.begin();
    while (avatarIterator != _avatarHash.end()) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarIterator.value());
        if (avatar != _myAvatar) {
            if (avatar->isInScene()) {
                avatar->removeFromScene(avatar, scene, transaction);
            }
            handleRemovedAvatar(avatar);
            avatarIterator = _avatarHash.erase(avatarIterator);
        } else {
            ++avatarIterator;
        }
    }
    assert(scene);
    scene->enqueueTransaction(transaction);
    _myAvatar->clearLookAtTargetAvatar();
}

void AvatarManager::deleteAllAvatars() {
    assert(_motionStates.empty()); // should have called clearOtherAvatars() before getting here
    deleteMotionStates();

    QReadLocker locker(&_hashLock);
    AvatarHash::iterator avatarIterator =  _avatarHash.begin();
    while (avatarIterator != _avatarHash.end()) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarIterator.value());
        avatarIterator = _avatarHash.erase(avatarIterator);
        avatar->die();
    }
}

void AvatarManager::deleteMotionStates() {
    // delete motionstates that were removed from physics last frame
    for (auto state : _motionStatesToDelete) {
        delete state;
    }
    _motionStatesToDelete.clear();
}

void AvatarManager::getObjectsToRemoveFromPhysics(VectorOfMotionStates& result) {
    deleteMotionStates();
    result = _motionStatesToRemoveFromPhysics;
    _motionStatesToDelete.swap(_motionStatesToRemoveFromPhysics);
}

void AvatarManager::getObjectsToAddToPhysics(VectorOfMotionStates& result) {
    result.clear();
    for (auto motionState : _motionStatesToAddToPhysics) {
        result.push_back(motionState);
    }
    _motionStatesToAddToPhysics.clear();
}

void AvatarManager::getObjectsToChange(VectorOfMotionStates& result) {
    result.clear();
    AvatarMotionStateMap::iterator motionStateItr = _motionStates.begin();
    while (motionStateItr != _motionStates.end()) {
        if ((*motionStateItr)->getIncomingDirtyFlags() != 0) {
            result.push_back(*motionStateItr);
        }
        ++motionStateItr;
    }
}

void AvatarManager::handleChangedMotionStates(const VectorOfMotionStates& motionStates) {
    // TODO: extract the MyAvatar results once we use a MotionState for it.
}

void AvatarManager::handleCollisionEvents(const CollisionEvents& collisionEvents) {
    for (Collision collision : collisionEvents) {
        // TODO: The plan is to handle MOTIONSTATE_TYPE_AVATAR, and then MOTIONSTATE_TYPE_MYAVATAR. As it is, other
        // people's avatars will have an id that doesn't match any entities, and one's own avatar will have
        // an id of null. Thus this code handles any collision in which one of the participating objects is
        // my avatar. (Other user machines will make a similar analysis and inject sound for their collisions.)
        if (collision.idA.isNull() || collision.idB.isNull()) {
            auto myAvatar = getMyAvatar();
            auto collisionSound = myAvatar->getCollisionSound();
            if (collisionSound) {
                const auto characterController = myAvatar->getCharacterController();
                const float avatarVelocityChange = (characterController ? glm::length(characterController->getVelocityChange()) : 0.0f);
                const float velocityChange = glm::length(collision.velocityChange) + avatarVelocityChange;
                const float MIN_AVATAR_COLLISION_ACCELERATION = 2.4f; // walking speed
                const bool isSound = (collision.type == CONTACT_EVENT_TYPE_START) && (velocityChange > MIN_AVATAR_COLLISION_ACCELERATION);

                if (!isSound) {
                    return;  // No sense iterating for others. We only have one avatar.
                }
                // Your avatar sound is personal to you, so let's say the "mass" part of the kinetic energy is already accounted for.
                const float energy = velocityChange * velocityChange;
                const float COLLISION_ENERGY_AT_FULL_VOLUME = 10.0f;
                const float energyFactorOfFull = fmin(1.0f, energy / COLLISION_ENERGY_AT_FULL_VOLUME);

                // For general entity collisionSoundURL, playSound supports changing the pitch for the sound based on the size of the object,
                // but most avatars are roughly the same size, so let's not be so fancy yet.
                const float AVATAR_STRETCH_FACTOR = 1.0f;

                _collisionInjectors.remove_if([](const AudioInjectorPointer& injector) {
                    return !injector || injector->isFinished();
                });

                static const int MAX_INJECTOR_COUNT = 3;
                if (_collisionInjectors.size() < MAX_INJECTOR_COUNT) {
                    auto injector = AudioInjector::playSound(collisionSound, energyFactorOfFull, AVATAR_STRETCH_FACTOR,
                                                             myAvatar->getPosition());
                    _collisionInjectors.emplace_back(injector);
                }
                myAvatar->collisionWithEntity(collision);
                return;
            }
        }
    }
}

void AvatarManager::updateAvatarRenderStatus(bool shouldRenderAvatars) {
    _shouldRender = shouldRenderAvatars;
    const render::ScenePointer& scene = qApp->getMain3DScene();
    render::Transaction transaction;
    if (_shouldRender) {
        for (auto avatarData : _avatarHash) {
            auto avatar = std::static_pointer_cast<Avatar>(avatarData);
            avatar->addToScene(avatar, scene, transaction);
        }
    } else {
        for (auto avatarData : _avatarHash) {
            auto avatar = std::static_pointer_cast<Avatar>(avatarData);
            avatar->removeFromScene(avatar, scene, transaction);
        }
    }
    scene->enqueueTransaction(transaction);
}

AvatarSharedPointer AvatarManager::getAvatarBySessionID(const QUuid& sessionID) const {
    if (sessionID == AVATAR_SELF_ID || sessionID == _myAvatar->getSessionUUID()) {
        return _myAvatar;
    }

    return findAvatar(sessionID);
}

RayToAvatarIntersectionResult AvatarManager::findRayIntersection(const PickRay& ray,
                                                                 const QScriptValue& avatarIdsToInclude,
                                                                 const QScriptValue& avatarIdsToDiscard) {
    QVector<EntityItemID> avatarsToInclude = qVectorEntityItemIDFromScriptValue(avatarIdsToInclude);
    QVector<EntityItemID> avatarsToDiscard = qVectorEntityItemIDFromScriptValue(avatarIdsToDiscard);

    return findRayIntersectionVector(ray, avatarsToInclude, avatarsToDiscard);
}

RayToAvatarIntersectionResult AvatarManager::findRayIntersectionVector(const PickRay& ray,
                                                                       const QVector<EntityItemID>& avatarsToInclude,
                                                                       const QVector<EntityItemID>& avatarsToDiscard) {
    RayToAvatarIntersectionResult result;
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(const_cast<AvatarManager*>(this), "findRayIntersectionVector",
                                  Q_RETURN_ARG(RayToAvatarIntersectionResult, result),
                                  Q_ARG(const PickRay&, ray),
                                  Q_ARG(const QVector<EntityItemID>&, avatarsToInclude),
                                  Q_ARG(const QVector<EntityItemID>&, avatarsToDiscard));
        return result;
    }

    glm::vec3 normDirection = glm::normalize(ray.direction);

    for (auto avatarData : _avatarHash) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarData);
        if ((avatarsToInclude.size() > 0 && !avatarsToInclude.contains(avatar->getID())) ||
            (avatarsToDiscard.size() > 0 && avatarsToDiscard.contains(avatar->getID()))) {
            continue;
        }

        float distance;
        BoxFace face;
        glm::vec3 surfaceNormal;

        SkeletonModelPointer avatarModel = avatar->getSkeletonModel();

        // It's better to intersect the ray against the avatar's actual mesh, but this is currently difficult to
        // do, because the transformed mesh data only exists over in GPU-land.  As a compromise, this code
        // intersects against the avatars capsule and then against the (T-pose) mesh.  The end effect is that picking
        // against the avatar is sort-of right, but you likely wont be able to pick against the arms.

        // TODO -- find a way to extract transformed avatar mesh data from the rendering engine.

        // if we weren't picking against the capsule, we would want to pick against the avatarBounds...
        // AABox avatarBounds = avatarModel->getRenderableMeshBound();
        // if (!avatarBounds.findRayIntersection(ray.origin, normDirection, distance, face, surfaceNormal)) {
        //     // ray doesn't intersect avatar's bounding-box
        //     continue;
        // }

        glm::vec3 start;
        glm::vec3 end;
        float radius;
        avatar->getCapsule(start, end, radius);
        bool intersects = findRayCapsuleIntersection(ray.origin, normDirection, start, end, radius, distance);
        if (!intersects) {
            // ray doesn't intersect avatar's capsule
            continue;
        }

        QString extraInfo;
        intersects = avatarModel->findRayIntersectionAgainstSubMeshes(ray.origin, normDirection,
                                                                      distance, face, surfaceNormal, extraInfo, true);

        if (intersects && (!result.intersects || distance < result.distance)) {
            result.intersects = true;
            result.avatarID = avatar->getID();
            result.distance = distance;
        }
    }

    if (result.intersects) {
        result.intersection = ray.origin + normDirection * result.distance;
    }

    return result;
}

// HACK
float AvatarManager::getAvatarSortCoefficient(const QString& name) {
    if (name == "size") {
        return AvatarData::_avatarSortCoefficientSize;
    } else if (name == "center") {
        return AvatarData::_avatarSortCoefficientCenter;
    } else if (name == "age") {
        return AvatarData::_avatarSortCoefficientAge;
    }
    return 0.0f;
}

// HACK
void AvatarManager::setAvatarSortCoefficient(const QString& name, const QScriptValue& value) {
    bool somethingChanged = false;
    if (value.isNumber()) {
        float numericalValue = (float)value.toNumber();
        if (name == "size") {
            AvatarData::_avatarSortCoefficientSize = numericalValue;
            somethingChanged = true;
        } else if (name == "center") {
            AvatarData::_avatarSortCoefficientCenter = numericalValue;
            somethingChanged = true;
        } else if (name == "age") {
            AvatarData::_avatarSortCoefficientAge = numericalValue;
            somethingChanged = true;
        }
    }
    if (somethingChanged) {
        size_t packetSize = sizeof(AvatarData::_avatarSortCoefficientSize) + 
                            sizeof(AvatarData::_avatarSortCoefficientCenter) +
                            sizeof(AvatarData::_avatarSortCoefficientAge);

        auto packet = NLPacket::create(PacketType::AdjustAvatarSorting, packetSize);
        packet->writePrimitive(AvatarData::_avatarSortCoefficientSize);
        packet->writePrimitive(AvatarData::_avatarSortCoefficientCenter);
        packet->writePrimitive(AvatarData::_avatarSortCoefficientAge);
        DependencyManager::get<NodeList>()->broadcastToNodes(std::move(packet), NodeSet() << NodeType::AvatarMixer);
    }
}
