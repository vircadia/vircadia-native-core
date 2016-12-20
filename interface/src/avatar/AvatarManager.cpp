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


#include <AvatarData.h>
#include <PerfStat.h>
#include <RegisteredMetaTypes.h>
#include <Rig.h>
#include <SettingHandle.h>
#include <UsersScriptingInterface.h>
#include <UUID.h>

#include "Application.h"
#include "Avatar.h"
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

static QScriptValue localLightToScriptValue(QScriptEngine* engine, const AvatarManager::LocalLight& light) {
    QScriptValue object = engine->newObject();
    object.setProperty("direction", vec3toScriptValue(engine, light.direction));
    object.setProperty("color", vec3toScriptValue(engine, light.color));
    return object;
}

static void localLightFromScriptValue(const QScriptValue& value, AvatarManager::LocalLight& light) {
    vec3FromScriptValue(value.property("direction"), light.direction);
    vec3FromScriptValue(value.property("color"), light.color);
}

void AvatarManager::registerMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, localLightToScriptValue, localLightFromScriptValue);
    qScriptRegisterSequenceMetaType<QVector<AvatarManager::LocalLight> >(engine);
}

AvatarManager::AvatarManager(QObject* parent) :
    _avatarFades(),
    _myAvatar(std::make_shared<MyAvatar>(std::make_shared<Rig>()))
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
    connect(nodeList.data(), "ignoredNode", this, "removeAvatar");
}

AvatarManager::~AvatarManager() {
}

void AvatarManager::init() {
    _myAvatar->init();
    {
        QWriteLocker locker(&_hashLock);
        _avatarHash.insert(MY_AVATAR_KEY, _myAvatar);
    }

    connect(DependencyManager::get<SceneScriptingInterface>().data(), &SceneScriptingInterface::shouldRenderAvatarsChanged,
            this, &AvatarManager::updateAvatarRenderStatus, Qt::QueuedConnection);

    render::ScenePointer scene = qApp->getMain3DScene();
    render::PendingChanges pendingChanges;
    if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderAvatars()) {
        _myAvatar->addToScene(_myAvatar, scene, pendingChanges);
    }
    scene->enqueuePendingChanges(pendingChanges);
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

void AvatarManager::updateOtherAvatars(float deltaTime) {
    // lock the hash for read to check the size
    QReadLocker lock(&_hashLock);

    if (_avatarHash.size() < 2 && _avatarFades.isEmpty()) {
        return;
    }

    lock.unlock();

    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "Application::updateAvatars()");

    PerformanceTimer perfTimer("otherAvatars");
    render::PendingChanges pendingChanges;

    // simulate avatars
    auto hashCopy = getHashCopy();

    uint64_t start = usecTimestampNow();
    AvatarHash::iterator avatarIterator = hashCopy.begin();
    while (avatarIterator != hashCopy.end()) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarIterator.value());

        if (avatar == _myAvatar || !avatar->isInitialized()) {
            // DO NOT update _myAvatar!  Its update has already been done earlier in the main loop.
            // DO NOT update or fade out uninitialized Avatars
            ++avatarIterator;
        } else if (avatar->shouldDie()) {
            removeAvatar(avatarIterator.key());
            ++avatarIterator;
        } else {
            avatar->simulate(deltaTime);
            ++avatarIterator;

            avatar->updateRenderItem(pendingChanges);
        }
    }
    qApp->getMain3DScene()->enqueuePendingChanges(pendingChanges);

    // simulate avatar fades
    simulateAvatarFades(deltaTime);

    PROFILE_COUNTER(simulation_avatar, "NumAvatarsPerSec",
            { { "NumAvatarsPerSec", (float)(size() * USECS_PER_SECOND) / (float)(usecTimestampNow() - start) } });
    PROFILE_COUNTER(simulation_avatar, "NumJointsPerSec", { { "NumJointsPerSec", Avatar::getNumJointsProcessedPerSecond() } });
}

void AvatarManager::postUpdate(float deltaTime) {
    auto hashCopy = getHashCopy();
    AvatarHash::iterator avatarIterator = hashCopy.begin();
    for (avatarIterator = hashCopy.begin(); avatarIterator != hashCopy.end(); avatarIterator++) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarIterator.value());
        avatar->postUpdate(deltaTime);
    }
}

void AvatarManager::simulateAvatarFades(float deltaTime) {
    QVector<AvatarSharedPointer>::iterator fadingIterator = _avatarFades.begin();

    const float SHRINK_RATE = 0.15f;
    const float MIN_FADE_SCALE = MIN_AVATAR_SCALE;

    render::ScenePointer scene = qApp->getMain3DScene();
    render::PendingChanges pendingChanges;
    while (fadingIterator != _avatarFades.end()) {
        auto avatar = std::static_pointer_cast<Avatar>(*fadingIterator);
        avatar->setTargetScale(avatar->getUniformScale() * SHRINK_RATE);
        if (avatar->getTargetScale() <= MIN_FADE_SCALE) {
            avatar->removeFromScene(*fadingIterator, scene, pendingChanges);
            // only remove from _avatarFades if we're sure its motionState has been removed from PhysicsEngine
            if (_motionStatesToRemoveFromPhysics.empty()) {
                fadingIterator = _avatarFades.erase(fadingIterator);
            } else {
                ++fadingIterator;
            }
        } else {
            avatar->simulate(deltaTime);
            ++fadingIterator;
        }
    }
    scene->enqueuePendingChanges(pendingChanges);
}

AvatarSharedPointer AvatarManager::newSharedAvatar() {
    return std::make_shared<Avatar>(std::make_shared<Rig>());
}

AvatarSharedPointer AvatarManager::addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) {
    auto newAvatar = AvatarHashMap::addAvatar(sessionUUID, mixerWeakPointer);
    auto rawRenderableAvatar = std::static_pointer_cast<Avatar>(newAvatar);

    render::ScenePointer scene = qApp->getMain3DScene();
    if (scene) {
        render::PendingChanges pendingChanges;
        if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderAvatars()) {
            rawRenderableAvatar->addToScene(rawRenderableAvatar, scene, pendingChanges);
        }
        scene->enqueuePendingChanges(pendingChanges);
    } else {
        qCWarning(interfaceapp) << "AvatarManager::addAvatar() : Unexpected null scene, possibly during application shutdown";
    }

    return newAvatar;
}

// virtual
void AvatarManager::removeAvatar(const QUuid& sessionUUID, KillAvatarReason removalReason) {
    QWriteLocker locker(&_hashLock);

    auto removedAvatar = _avatarHash.take(sessionUUID);
    if (removedAvatar) {
        handleRemovedAvatar(removedAvatar, removalReason);
    }
}

void AvatarManager::handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason) {
    AvatarHashMap::handleRemovedAvatar(removedAvatar);

    // removedAvatar is a shared pointer to an AvatarData but we need to get to the derived Avatar
    // class in this context so we can call methods that don't exist at the base class.
    Avatar* avatar = static_cast<Avatar*>(removedAvatar.get());
    avatar->die();

    AvatarMotionState* motionState = avatar->getMotionState();
    if (motionState) {
        _motionStatesThatMightUpdate.remove(motionState);
        _motionStatesToAddToPhysics.remove(motionState);
        _motionStatesToRemoveFromPhysics.push_back(motionState);
    }

    if (removalReason == KillAvatarReason::TheirAvatarEnteredYourBubble) {
        emit DependencyManager::get<UsersScriptingInterface>()->enteredIgnoreRadius();
    }
    _avatarFades.push_back(removedAvatar);
}

void AvatarManager::clearOtherAvatars() {
    // clear any avatars that came from an avatar-mixer
    QWriteLocker locker(&_hashLock);

    AvatarHash::iterator avatarIterator =  _avatarHash.begin();
    while (avatarIterator != _avatarHash.end()) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarIterator.value());
        if (avatar == _myAvatar || !avatar->isInitialized()) {
            // don't remove myAvatar or uninitialized avatars from the list
            ++avatarIterator;
        } else {
            auto removedAvatar = avatarIterator.value();
            avatarIterator = _avatarHash.erase(avatarIterator);

            handleRemovedAvatar(removedAvatar);
        }
    }
    _myAvatar->clearLookAtTargetAvatar();
}

void AvatarManager::clearAllAvatars() {
    clearOtherAvatars();

    QWriteLocker locker(&_hashLock);

    handleRemovedAvatar(_myAvatar);
}

void AvatarManager::setLocalLights(const QVector<AvatarManager::LocalLight>& localLights) {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "setLocalLights", Q_ARG(const QVector<AvatarManager::LocalLight>&, localLights));
        return;
    }
    _localLights = localLights;
}

QVector<AvatarManager::LocalLight> AvatarManager::getLocalLights() const {
    if (QThread::currentThread() != thread()) {
        QVector<AvatarManager::LocalLight> result;
        QMetaObject::invokeMethod(const_cast<AvatarManager*>(this), "getLocalLights", Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QVector<AvatarManager::LocalLight>, result));
        return result;
    }
    return _localLights;
}

void AvatarManager::getObjectsToRemoveFromPhysics(VectorOfMotionStates& result) {
    result.clear();
    result.swap(_motionStatesToRemoveFromPhysics);
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
    for (auto state : _motionStatesThatMightUpdate) {
        if (state->_dirtyFlags > 0) {
            result.push_back(state);
        }
    }
}

void AvatarManager::handleOutgoingChanges(const VectorOfMotionStates& motionStates) {
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


                _collisionInjectors.remove_if([](QPointer<AudioInjector>& injector) {
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

void AvatarManager::addAvatarToSimulation(Avatar* avatar) {
    assert(!avatar->getMotionState());

    ShapeInfo shapeInfo;
    avatar->computeShapeInfo(shapeInfo);
    btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
    if (shape) {
        // we don't add to the simulation now, we put it on a list to be added later
        AvatarMotionState* motionState = new AvatarMotionState(avatar, shape);
        avatar->setMotionState(motionState);
        _motionStatesToAddToPhysics.insert(motionState);
        _motionStatesThatMightUpdate.insert(motionState);
    }
}

void AvatarManager::updateAvatarRenderStatus(bool shouldRenderAvatars) {
    if (DependencyManager::get<SceneScriptingInterface>()->shouldRenderAvatars()) {
        for (auto avatarData : _avatarHash) {
            auto avatar = std::static_pointer_cast<Avatar>(avatarData);
            render::ScenePointer scene = qApp->getMain3DScene();
            render::PendingChanges pendingChanges;
            avatar->addToScene(avatar, scene, pendingChanges);
            scene->enqueuePendingChanges(pendingChanges);
        }
    } else {
        for (auto avatarData : _avatarHash) {
            auto avatar = std::static_pointer_cast<Avatar>(avatarData);
            render::ScenePointer scene = qApp->getMain3DScene();
            render::PendingChanges pendingChanges;
            avatar->removeFromScene(avatar, scene, pendingChanges);
            scene->enqueuePendingChanges(pendingChanges);
        }
    }
}


AvatarSharedPointer AvatarManager::getAvatarBySessionID(const QUuid& sessionID) {
    if (sessionID == AVATAR_SELF_ID || sessionID == _myAvatar->getSessionUUID()) {
        return _myAvatar;
    }

    return findAvatar(sessionID);
}

RayToAvatarIntersectionResult AvatarManager::findRayIntersection(const PickRay& ray,
                                                                 const QScriptValue& avatarIdsToInclude,
                                                                 const QScriptValue& avatarIdsToDiscard) {
    RayToAvatarIntersectionResult result;
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(const_cast<AvatarManager*>(this), "findRayIntersection", Qt::BlockingQueuedConnection,
                                  Q_RETURN_ARG(RayToAvatarIntersectionResult, result),
                                  Q_ARG(const PickRay&, ray),
                                  Q_ARG(const QScriptValue&, avatarIdsToInclude),
                                  Q_ARG(const QScriptValue&, avatarIdsToDiscard));
        return result;
    }

    QVector<EntityItemID> avatarsToInclude = qVectorEntityItemIDFromScriptValue(avatarIdsToInclude);
    QVector<EntityItemID> avatarsToDiscard = qVectorEntityItemIDFromScriptValue(avatarIdsToDiscard);

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
