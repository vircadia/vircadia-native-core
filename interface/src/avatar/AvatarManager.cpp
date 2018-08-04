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

#include "AvatarManager.h"

#include <string>

#include <QScriptEngine>
#include <QtCore/QJsonDocument>

#include "AvatarLogging.h"

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
#include <PrioritySortUtil.h>
#include <RegisteredMetaTypes.h>
#include <Rig.h>
#include <SettingHandle.h>
#include <UsersScriptingInterface.h>
#include <UUID.h>
#include <shared/ConicalViewFrustum.h>

#include "Application.h"
#include "InterfaceLogging.h"
#include "Menu.h"
#include "MyAvatar.h"
#include "OtherAvatar.h"
#include "SceneScriptingInterface.h"

// 50 times per second - target is 45hz, but this helps account for any small deviations
// in the update loop - this also results in ~30hz when in desktop mode which is essentially
// what we want
const int CLIENT_TO_AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND = 50;
static const quint64 MIN_TIME_BETWEEN_MY_AVATAR_DATA_SENDS = USECS_PER_SECOND / CLIENT_TO_AVATAR_MIXER_BROADCAST_FRAMES_PER_SECOND;

// We add _myAvatar into the hash with all the other AvatarData, and we use the default NULL QUid as the key.
const QUuid MY_AVATAR_KEY;  // NULL key

namespace {
    // For an unknown avatar-data packet, wait this long before requesting the identity.
    constexpr std::chrono::milliseconds REQUEST_UNKNOWN_IDENTITY_DELAY { 5 * 1000 };
    constexpr int REQUEST_UNKNOWN_IDENTITY_TRANSMITS = 3;
}
using std::chrono::steady_clock;

AvatarManager::AvatarManager(QObject* parent) :
    _avatarsToFade(),
    _myAvatar(new MyAvatar(qApp->thread()), [](MyAvatar* ptr) { ptr->deleteLater(); })
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
    render::Transaction transaction;
    _myAvatar->updateRenderItem(transaction);
    qApp->getMain3DScene()->enqueueTransaction(transaction);

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

    class SortableAvatar: public PrioritySortUtil::Sortable {
    public:
        SortableAvatar() = delete;
        SortableAvatar(const AvatarSharedPointer& avatar) : _avatar(avatar) {}
        glm::vec3 getPosition() const override { return _avatar->getWorldPosition(); }
        float getRadius() const override { return std::static_pointer_cast<Avatar>(_avatar)->getBoundingRadius(); }
        uint64_t getTimestamp() const override { return std::static_pointer_cast<Avatar>(_avatar)->getLastRenderUpdateTime(); }
        AvatarSharedPointer getAvatar() const { return _avatar; }
    private:
        AvatarSharedPointer _avatar;
    };


    const auto& views = qApp->getConicalViews();
    PrioritySortUtil::PriorityQueue<SortableAvatar> sortedAvatars(views,
            AvatarData::_avatarSortCoefficientSize,
            AvatarData::_avatarSortCoefficientCenter,
            AvatarData::_avatarSortCoefficientAge);

    // sort
    auto avatarMap = getHashCopy();
    AvatarHash::iterator itr = avatarMap.begin();
    while (itr != avatarMap.end()) {
        const auto& avatar = std::static_pointer_cast<Avatar>(*itr);
        // DO NOT update _myAvatar!  Its update has already been done earlier in the main loop.
        // DO NOT update or fade out uninitialized Avatars
        if (avatar != _myAvatar && avatar->isInitialized()) {
            sortedAvatars.push(SortableAvatar(avatar));
        }
        ++itr;
    }

    // process in sorted order
    uint64_t startTime = usecTimestampNow();
    const uint64_t UPDATE_BUDGET = 2000; // usec
    uint64_t updateExpiry = startTime + UPDATE_BUDGET;
    int numAvatarsUpdated = 0;
    int numAVatarsNotUpdated = 0;
    bool physicsEnabled = qApp->isPhysicsEnabled();

    render::Transaction transaction;
    while (!sortedAvatars.empty()) {
        const SortableAvatar& sortData = sortedAvatars.top();
        const auto avatar = std::static_pointer_cast<Avatar>(sortData.getAvatar());
        const auto otherAvatar = std::static_pointer_cast<OtherAvatar>(sortData.getAvatar());

        // if the geometry is loaded then turn off the orb
        if (avatar->getSkeletonModel()->isLoaded()) {
            // remove the orb if it is there
            otherAvatar->removeOrb();
        } else {
            otherAvatar->updateOrbPosition();
        }

        bool ignoring = DependencyManager::get<NodeList>()->isPersonalMutingNode(avatar->getID());
        if (ignoring) {
            sortedAvatars.pop();
            continue;
        }

        // for ALL avatars...
        if (_shouldRender) {
            avatar->ensureInScene(avatar, qApp->getMain3DScene());
        }
        if (physicsEnabled && !avatar->isInPhysicsSimulation()) {
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
            bool inView = sortData.getPriority() > OUT_OF_VIEW_THRESHOLD;
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
            bool inView = sortData.getPriority() > OUT_OF_VIEW_THRESHOLD;
            if (!inView) {
                break;
            }
            if (inView && avatar->hasNewJointData()) {
                numAVatarsNotUpdated++;
            }
            sortedAvatars.pop();
            while (inView && !sortedAvatars.empty()) {
                const SortableAvatar& newSortData = sortedAvatars.top();
                const auto newAvatar = std::static_pointer_cast<Avatar>(newSortData.getAvatar());
                inView = newSortData.getPriority() > OUT_OF_VIEW_THRESHOLD;
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

    _numAvatarsUpdated = numAvatarsUpdated;
    _numAvatarsNotUpdated = numAVatarsNotUpdated;

    simulateAvatarFades(deltaTime);

    // Check on avatars with pending identities:
    steady_clock::time_point now = steady_clock::now();
    QWriteLocker writeLock(&_hashLock);
    for (auto pendingAvatar = _pendingAvatars.begin(); pendingAvatar != _pendingAvatars.end(); ++pendingAvatar) {
        if (now - pendingAvatar->creationTime >= REQUEST_UNKNOWN_IDENTITY_DELAY) {
            // Too long without an ID
            sendIdentityRequest(pendingAvatar->avatar->getID());
            if (++pendingAvatar->transmits >= REQUEST_UNKNOWN_IDENTITY_TRANSMITS) {
                qCDebug(avatars) << "Requesting identity for unknown avatar (final request)" <<
                    pendingAvatar->avatar->getID().toString();

                pendingAvatar = _pendingAvatars.erase(pendingAvatar);
                if (pendingAvatar == _pendingAvatars.end()) {
                    break;
                }
            } else {
                pendingAvatar->creationTime = now;
                qCDebug(avatars) << "Requesting identity for unknown avatar" << pendingAvatar->avatar->getID().toString();
            }
        }
    }

    _avatarSimulationTime = (float)(usecTimestampNow() - startTime) / (float)USECS_PER_MSEC;
}

void AvatarManager::postUpdate(float deltaTime, const render::ScenePointer& scene) {
    auto hashCopy = getHashCopy();
    AvatarHash::iterator avatarIterator = hashCopy.begin();
    for (avatarIterator = hashCopy.begin(); avatarIterator != hashCopy.end(); avatarIterator++) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarIterator.value());
        avatar->postUpdate(deltaTime, scene);
    }
}

void AvatarManager::sendIdentityRequest(const QUuid& avatarID) const {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->eachMatchingNode(
        [&](const SharedNodePointer& node)->bool {
        return node->getType() == NodeType::AvatarMixer && node->getActiveSocket();
    },
        [&](const SharedNodePointer& node) {
        auto packet = NLPacket::create(PacketType::AvatarIdentityRequest, NUM_BYTES_RFC4122_UUID, true);
        packet->write(avatarID.toRfc4122());
        nodeList->sendPacket(std::move(packet), *node);
        ++_identityRequestsSent;
    });
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
    return AvatarSharedPointer(new OtherAvatar(qApp->thread()), [](OtherAvatar* ptr) { ptr->deleteLater(); });
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
                                                             myAvatar->getWorldPosition());
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
    auto avatarHashCopy = getHashCopy();
    if (_shouldRender) {
        for (auto avatarData : avatarHashCopy) {
            auto avatar = std::static_pointer_cast<Avatar>(avatarData);
            avatar->addToScene(avatar, scene, transaction);
        }
    } else {
        for (auto avatarData : avatarHashCopy) {
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

    auto avatarHashCopy = getHashCopy();
    for (auto avatarData : avatarHashCopy) {
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

        QVariantMap extraInfo;
        intersects = avatarModel->findRayIntersectionAgainstSubMeshes(ray.origin, normDirection,
                                                                      distance, face, surfaceNormal, extraInfo, true);

        if (intersects && (!result.intersects || distance < result.distance)) {
            result.intersects = true;
            result.avatarID = avatar->getID();
            result.distance = distance;
            result.face = face;
            result.surfaceNormal = surfaceNormal;
            result.extraInfo = extraInfo;
        }
    }

    if (result.intersects) {
        result.intersection = ray.origin + normDirection * result.distance;
    }

    return result;
}

ParabolaToAvatarIntersectionResult AvatarManager::findParabolaIntersectionVector(const PickParabola& pick,
                                                                                 const QVector<EntityItemID>& avatarsToInclude,
                                                                                 const QVector<EntityItemID>& avatarsToDiscard) {
    ParabolaToAvatarIntersectionResult result;
    if (QThread::currentThread() != thread()) {
        BLOCKING_INVOKE_METHOD(const_cast<AvatarManager*>(this), "findParabolaIntersectionVector",
                               Q_RETURN_ARG(ParabolaToAvatarIntersectionResult, result),
                               Q_ARG(const PickParabola&, pick),
                               Q_ARG(const QVector<EntityItemID>&, avatarsToInclude),
                               Q_ARG(const QVector<EntityItemID>&, avatarsToDiscard));
        return result;
    }

    auto avatarHashCopy = getHashCopy();
    for (auto avatarData : avatarHashCopy) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarData);
        if ((avatarsToInclude.size() > 0 && !avatarsToInclude.contains(avatar->getID())) ||
            (avatarsToDiscard.size() > 0 && avatarsToDiscard.contains(avatar->getID()))) {
            continue;
        }

        float parabolicDistance;
        BoxFace face;
        glm::vec3 surfaceNormal;

        SkeletonModelPointer avatarModel = avatar->getSkeletonModel();

        // It's better to intersect the parabola against the avatar's actual mesh, but this is currently difficult to
        // do, because the transformed mesh data only exists over in GPU-land.  As a compromise, this code
        // intersects against the avatars capsule and then against the (T-pose) mesh.  The end effect is that picking
        // against the avatar is sort-of right, but you likely wont be able to pick against the arms.

        // TODO -- find a way to extract transformed avatar mesh data from the rendering engine.

        // if we weren't picking against the capsule, we would want to pick against the avatarBounds...
        // AABox avatarBounds = avatarModel->getRenderableMeshBound();
        // if (!avatarBounds.findParabolaIntersection(pick.origin, pick.velocity, pick.acceleration, parabolicDistance, face, surfaceNormal)) {
        //     // parabola doesn't intersect avatar's bounding-box
        //     continue;
        // }

        glm::vec3 start;
        glm::vec3 end;
        float radius;
        avatar->getCapsule(start, end, radius);
        bool intersects = findParabolaCapsuleIntersection(pick.origin, pick.velocity, pick.acceleration, start, end, radius, avatar->getWorldOrientation(), parabolicDistance);
        if (!intersects) {
            // ray doesn't intersect avatar's capsule
            continue;
        }

        QVariantMap extraInfo;
        intersects = avatarModel->findParabolaIntersectionAgainstSubMeshes(pick.origin, pick.velocity, pick.acceleration,
                                                                           parabolicDistance, face, surfaceNormal, extraInfo, true);

        if (intersects && (!result.intersects || parabolicDistance < result.parabolicDistance)) {
            result.intersects = true;
            result.avatarID = avatar->getID();
            result.parabolicDistance = parabolicDistance;
            result.face = face;
            result.surfaceNormal = surfaceNormal;
            result.extraInfo = extraInfo;
        }
    }

    if (result.intersects) {
        result.intersection = pick.origin + pick.velocity * result.parabolicDistance + 0.5f * pick.acceleration * result.parabolicDistance * result.parabolicDistance;
        result.distance = glm::distance(pick.origin, result.intersection);
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

 QVariantMap AvatarManager::getPalData(const QList<QString> specificAvatarIdentifiers) {
    QJsonArray palData;

    auto avatarMap = getHashCopy();
    AvatarHash::iterator itr = avatarMap.begin();
    while (itr != avatarMap.end()) {
        std::shared_ptr<Avatar> avatar = std::static_pointer_cast<Avatar>(*itr);
        QString currentSessionUUID = avatar->getSessionUUID().toString();
        if (specificAvatarIdentifiers.isEmpty() || specificAvatarIdentifiers.contains(currentSessionUUID)) {
            QJsonObject thisAvatarPalData;
            
            auto myAvatar = DependencyManager::get<AvatarManager>()->getMyAvatar();

            if (currentSessionUUID == myAvatar->getSessionUUID().toString()) {
                currentSessionUUID = "";
            }
            
            thisAvatarPalData.insert("sessionUUID", currentSessionUUID);
            thisAvatarPalData.insert("sessionDisplayName", avatar->getSessionDisplayName());
            thisAvatarPalData.insert("audioLoudness", avatar->getAudioLoudness());
            thisAvatarPalData.insert("isReplicated", avatar->getIsReplicated());

            glm::vec3 position = avatar->getWorldPosition();
            QJsonObject jsonPosition;
            jsonPosition.insert("x", position.x);
            jsonPosition.insert("y", position.y);
            jsonPosition.insert("z", position.z);
            thisAvatarPalData.insert("position", jsonPosition);

            float palOrbOffset = 0.2f;
            int headIndex = avatar->getJointIndex("Head");
            if (headIndex > 0) {
                glm::vec3 jointTranslation = avatar->getAbsoluteJointTranslationInObjectFrame(headIndex);
                palOrbOffset = jointTranslation.y / 2;
            }
            thisAvatarPalData.insert("palOrbOffset", palOrbOffset);

            palData.append(thisAvatarPalData);
        }
        ++itr;
    }
    QJsonObject doc;
    doc.insert("data", palData);
    return doc.toVariantMap();
}
