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

    // when we hear that the user has ignored an avatar by session UUID
    // immediately remove that avatar instead of waiting for the absence of packets from avatar mixer
    connect(nodeList.data(), &NodeList::ignoredNode, this, [this](const QUuid& nodeID, bool enabled) {
        if (enabled) {
            removeAvatar(nodeID, KillAvatarReason::AvatarIgnored);
        }
    });

    const float AVATAR_TRANSIT_TRIGGER_DISTANCE = 1.0f;
    const int AVATAR_TRANSIT_FRAME_COUNT = 11; // Based on testing
    const int AVATAR_TRANSIT_FRAMES_PER_METER = 3; // Based on testing

    const QString START_ANIMATION_URL = "https://hifi-content.s3.amazonaws.com/luis/test_scripts/transitApp/animations/teleport01_warp.fbx";
    const QString MIDDLE_ANIMATION_URL = "https://hifi-content.s3.amazonaws.com/luis/test_scripts/transitApp/animations/teleport01_warp.fbx";
    const QString END_ANIMATION_URL = "https://hifi-content.s3.amazonaws.com/luis/test_scripts/transitApp/animations/teleport01_warp.fbx";

    _transitConfig._totalFrames = AVATAR_TRANSIT_FRAME_COUNT;
    _transitConfig._triggerDistance = AVATAR_TRANSIT_TRIGGER_DISTANCE;
    _transitConfig._framesPerMeter = AVATAR_TRANSIT_FRAMES_PER_METER;
    _transitConfig._isDistanceBased = true;
    
    _transitConfig._startTransitAnimation = AvatarTransit::TransitAnimation(START_ANIMATION_URL, 0, 14);
    _transitConfig._middleTransitAnimation = AvatarTransit::TransitAnimation(MIDDLE_ANIMATION_URL, 15, 0);
    _transitConfig._endTransitAnimation = AvatarTransit::TransitAnimation(END_ANIMATION_URL, 16, 38);
    
}

AvatarSharedPointer AvatarManager::addAvatar(const QUuid& sessionUUID, const QWeakPointer<Node>& mixerWeakPointer) {
    AvatarSharedPointer avatar = AvatarHashMap::addAvatar(sessionUUID, mixerWeakPointer);

    const auto otherAvatar = std::static_pointer_cast<OtherAvatar>(avatar);
    if (otherAvatar && _space) {
        std::unique_lock<std::mutex> lock(_spaceLock);
        auto spaceIndex = _space->allocateID();
        otherAvatar->setSpaceIndex(spaceIndex);
        workload::Sphere sphere(otherAvatar->getWorldPosition(), otherAvatar->getBoundingRadius());
        workload::Transaction transaction;
        SpatiallyNestablePointer nestable = std::static_pointer_cast<SpatiallyNestable>(otherAvatar);
        transaction.reset(spaceIndex, sphere, workload::Owner(nestable));
        _space->enqueueTransaction(transaction);
    }
    return avatar;
}

AvatarManager::~AvatarManager() {
    assert(_avatarsToChangeInPhysics.empty());
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

void AvatarManager::setSpace(workload::SpacePointer& space ) {
    assert(!_space);
    _space = space;
}

void AvatarManager::playTransitAnimations(AvatarTransit::Status status) {
    auto startAnimation = _transitConfig._startTransitAnimation;
    auto middleAnimation = _transitConfig._middleTransitAnimation;
    auto endAnimation = _transitConfig._endTransitAnimation;
    
    const float REFERENCE_FPS = 30.0f;

    switch (status) {
    case AvatarTransit::Status::START_FRAME:
        qDebug() << "START_FRAME";
        _myAvatar->overrideAnimation(startAnimation._animationUrl, REFERENCE_FPS, false, startAnimation._firstFrame, startAnimation._firstFrame + startAnimation._frameCount);
        break;
    case AvatarTransit::Status::START_TRANSIT:
        qDebug() << "START_TRANSIT";
        _myAvatar->overrideAnimation(middleAnimation._animationUrl, REFERENCE_FPS, false, middleAnimation._firstFrame, middleAnimation._firstFrame + middleAnimation._frameCount);
        break;
    case AvatarTransit::Status::END_TRANSIT:
        qDebug() << "END_TRANSIT";
        _myAvatar->overrideAnimation(endAnimation._animationUrl, REFERENCE_FPS, false, endAnimation._firstFrame, endAnimation._firstFrame + endAnimation._frameCount);
        break;
    case AvatarTransit::Status::END_FRAME:
        qDebug() << "END_FRAME";
        _myAvatar->restoreAnimation();
        break;
    case AvatarTransit::Status::IDLE:
        break;
    case AvatarTransit::Status::TRANSITING:
        break;
    }

}

void AvatarManager::updateMyAvatar(float deltaTime) {
    bool showWarnings = Menu::getInstance()->isOptionChecked(MenuOption::PipelineWarnings);
    PerformanceWarning warn(showWarnings, "AvatarManager::updateMyAvatar()");

    AvatarTransit::Status status = _myAvatar->updateTransit(deltaTime, _myAvatar->getWorldPosition(), _transitConfig);
    if (status != AvatarTransit::Status::IDLE && status != AvatarTransit::Status::TRANSITING) {
        QUuid& transitEffectID = _myAvatar->getTransitEffectID();
        //changeAvatarTransitState(status, _myAvatar->getID(), transitEffectID);
    }
    if (_transitConfig._playAnimation) {
        playTransitAnimations(status);
    }
    
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

    auto avatarMap = getHashCopy();
    AvatarHash::iterator itr = avatarMap.begin();

    const auto& views = qApp->getConicalViews();
    PrioritySortUtil::PriorityQueue<SortableAvatar> sortedAvatars(views,
            AvatarData::_avatarSortCoefficientSize,
            AvatarData::_avatarSortCoefficientCenter,
            AvatarData::_avatarSortCoefficientAge);
    sortedAvatars.reserve(avatarMap.size() - 1); // don't include MyAvatar

    // sort
    while (itr != avatarMap.end()) {
        const auto& avatar = std::static_pointer_cast<Avatar>(*itr);
        // DO NOT update _myAvatar!  Its update has already been done earlier in the main loop.
        // DO NOT update or fade out uninitialized Avatars
        if (avatar != _myAvatar && avatar->isInitialized()) {
            sortedAvatars.push(SortableAvatar(avatar));
        }
        ++itr;
    }
    const auto& sortedAvatarVector = sortedAvatars.getSortedVector();

    // process in sorted order
    uint64_t startTime = usecTimestampNow();
    const uint64_t UPDATE_BUDGET = 2000; // usec
    uint64_t updateExpiry = startTime + UPDATE_BUDGET;
    int numAvatarsUpdated = 0;
    int numAVatarsNotUpdated = 0;

    render::Transaction renderTransaction;
    workload::Transaction workloadTransaction;
    for (auto it = sortedAvatarVector.begin(); it != sortedAvatarVector.end(); ++it) {
        const SortableAvatar& sortData = *it;
        const auto avatar = std::static_pointer_cast<OtherAvatar>(sortData.getAvatar());

        // TODO: to help us scale to more avatars it would be nice to not have to poll orb state here
        // if the geometry is loaded then turn off the orb
        if (avatar->getSkeletonModel()->isLoaded()) {
            // remove the orb if it is there
            avatar->removeOrb();
        } else {
            avatar->updateOrbPosition();
        }

        bool ignoring = DependencyManager::get<NodeList>()->isPersonalMutingNode(avatar->getID());
        if (ignoring) {
            continue;
        }

        // for ALL avatars...
        if (_shouldRender) {
            avatar->ensureInScene(avatar, qApp->getMain3DScene());
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
            // smooth other avatars positions
            {
                auto status = avatar->_transit.update(deltaTime, avatar->_globalPosition, _transitConfig);
                if (status != AvatarTransit::Status::IDLE && status != AvatarTransit::Status::TRANSITING) {
                    QUuid& transitEffectID = avatar->getTransitEffectID();
                    changeAvatarTransitState(status, avatar->getID(), transitEffectID);
                }
            }

            avatar->simulate(deltaTime, inView);
            avatar->updateRenderItem(renderTransaction);
            avatar->updateSpaceProxy(workloadTransaction);
            avatar->setLastRenderUpdateTime(startTime);
        } else {
            // we've spent our full time budget --> bail on the rest of the avatar updates
            // --> more avatars may freeze until their priority trickles up
            // --> some scale animations may glitch
            // --> some avatar velocity measurements may be a little off

            // no time to simulate, but we take the time to count how many were tragically missed
            while (it != sortedAvatarVector.end()) {
                const SortableAvatar& newSortData = *it;
                const auto newAvatar = std::static_pointer_cast<Avatar>(newSortData.getAvatar());
                bool inView = newSortData.getPriority() > OUT_OF_VIEW_THRESHOLD;
                // Once we reach an avatar that's not in view, all avatars after it will also be out of view
                if (!inView) {
                    break;
                }
                numAVatarsNotUpdated += (int)(newAvatar->hasNewJointData());
                ++it;
            }
            break;
        }
    }

    if (_shouldRender) {
        qApp->getMain3DScene()->enqueueTransaction(renderTransaction);
    }

    if (!_spaceProxiesToDelete.empty() && _space) {
        std::unique_lock<std::mutex> lock(_spaceLock);
        workloadTransaction.remove(_spaceProxiesToDelete);
        _spaceProxiesToDelete.clear();
    }
    _space->enqueueTransaction(workloadTransaction);

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
    QWeakPointer<NodeList> nodeListWeak = nodeList;
    nodeList->eachMatchingNode(
        [](const SharedNodePointer& node)->bool {
            return node->getType() == NodeType::AvatarMixer && node->getActiveSocket();
        },
        [this, avatarID, nodeListWeak](const SharedNodePointer& node) {
            auto nodeList = nodeListWeak.lock();
            if (nodeList) {
                auto packet = NLPacket::create(PacketType::AvatarIdentityRequest, NUM_BYTES_RFC4122_UUID, true);
                packet->write(avatarID.toRfc4122());
                nodeList->sendPacket(std::move(packet), *node);
                ++_identityRequestsSent;
            }
        }
    );
}

void AvatarManager::simulateAvatarFades(float deltaTime) {
    if (_avatarsToFade.empty()) {
        return;
    }

    QReadLocker locker(&_hashLock);
    QVector<AvatarSharedPointer>::iterator avatarItr = _avatarsToFade.begin();
    const render::ScenePointer& scene = qApp->getMain3DScene();
    render::Transaction transaction;
    while (avatarItr != _avatarsToFade.end()) {
        auto avatar = std::static_pointer_cast<Avatar>(*avatarItr);
        avatar->updateFadingStatus(scene);
        if (!avatar->isFading()) {
            // fading to zero is such a rare event we push a unique transaction for each
            if (avatar->isInScene()) {
                avatar->removeFromScene(*avatarItr, scene, transaction);
            }
            avatarItr = _avatarsToFade.erase(avatarItr);
        } else {
            ++avatarItr;
        }
    }
    scene->enqueueTransaction(transaction);
}

AvatarSharedPointer AvatarManager::newSharedAvatar() {
    return AvatarSharedPointer(new OtherAvatar(qApp->thread()), [](OtherAvatar* ptr) { ptr->deleteLater(); });
}

void AvatarManager::queuePhysicsChange(const OtherAvatarPointer& avatar) {
    _avatarsToChangeInPhysics.insert(avatar);
}

void AvatarManager::buildPhysicsTransaction(PhysicsEngine::Transaction& transaction) {
    SetOfOtherAvatars failedShapeBuilds;
    for (auto avatar : _avatarsToChangeInPhysics) {
        bool isInPhysics = avatar->isInPhysicsSimulation();
        if (isInPhysics != avatar->shouldBeInPhysicsSimulation()) {
            if (isInPhysics) {
                transaction.objectsToRemove.push_back(avatar->_motionState);
                avatar->_motionState = nullptr;
            } else {
                ShapeInfo shapeInfo;
                avatar->computeShapeInfo(shapeInfo);
                btCollisionShape* shape = const_cast<btCollisionShape*>(ObjectMotionState::getShapeManager()->getShape(shapeInfo));
                if (shape) {
                    AvatarMotionState* motionState = new AvatarMotionState(avatar, shape);
                    motionState->setMass(avatar->computeMass());
                    avatar->_motionState = motionState;
                    transaction.objectsToAdd.push_back(motionState);
                } else {
                    failedShapeBuilds.insert(avatar);
                }
            }
        } else if (isInPhysics) {
            transaction.objectsToChange.push_back(avatar->_motionState);
        }
    }
    _avatarsToChangeInPhysics.swap(failedShapeBuilds);
}

void AvatarManager::handleProcessedPhysicsTransaction(PhysicsEngine::Transaction& transaction) {
    // things on objectsToChange correspond to failed changes
    // so we push them back onto _avatarsToChangeInPhysics
    for (auto object : transaction.objectsToChange) {
        AvatarMotionState* motionState = static_cast<AvatarMotionState*>(object);
        assert(motionState);
        assert(motionState->_avatar);
        _avatarsToChangeInPhysics.insert(motionState->_avatar);
    }
    // things on objectsToRemove are ready for delete
    for (auto object : transaction.objectsToRemove) {
        delete object;
    }
    transaction.clear();
}

void AvatarManager::handleRemovedAvatar(const AvatarSharedPointer& removedAvatar, KillAvatarReason removalReason) {
    auto avatar = std::static_pointer_cast<OtherAvatar>(removedAvatar);
    {
        std::unique_lock<std::mutex> lock(_spaceLock);
        _spaceProxiesToDelete.push_back(avatar->getSpaceIndex());
    }
    AvatarHashMap::handleRemovedAvatar(avatar, removalReason);

    avatar->die();
    queuePhysicsChange(avatar);

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
    assert(_avatarsToChangeInPhysics.empty());

    QReadLocker locker(&_hashLock);
    AvatarHash::iterator avatarIterator =  _avatarHash.begin();
    while (avatarIterator != _avatarHash.end()) {
        auto avatar = std::static_pointer_cast<OtherAvatar>(avatarIterator.value());
        avatarIterator = _avatarHash.erase(avatarIterator);
        avatar->die();
        assert(!avatar->_motionState);
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

    // It's better to intersect the ray against the avatar's actual mesh, but this is currently difficult to
    // do, because the transformed mesh data only exists over in GPU-land.  As a compromise, this code
    // intersects against the avatars capsule and then against the (T-pose) mesh.  The end effect is that picking
    // against the avatar is sort-of right, but you likely wont be able to pick against the arms.

    // TODO -- find a way to extract transformed avatar mesh data from the rendering engine.

    std::vector<SortedAvatar> sortedAvatars;
    auto avatarHashCopy = getHashCopy();
    for (auto avatarData : avatarHashCopy) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarData);
        if ((avatarsToInclude.size() > 0 && !avatarsToInclude.contains(avatar->getID())) ||
            (avatarsToDiscard.size() > 0 && avatarsToDiscard.contains(avatar->getID()))) {
            continue;
        }

        float distance = FLT_MAX;
#if 0
        // if we weren't picking against the capsule, we would want to pick against the avatarBounds...
        SkeletonModelPointer avatarModel = avatar->getSkeletonModel();
        AABox avatarBounds = avatarModel->getRenderableMeshBound();
        if (avatarBounds.contains(ray.origin)) {
            distance = 0.0f;
        } else {
            float boundDistance = FLT_MAX;
            BoxFace face;
            glm::vec3 surfaceNormal;
            if (avatarBounds.findRayIntersection(ray.origin, ray.direction, boundDistance, face, surfaceNormal)) {
                distance = boundDistance;
            }
        }
#else
        glm::vec3 start;
        glm::vec3 end;
        float radius;
        avatar->getCapsule(start, end, radius);
        findRayCapsuleIntersection(ray.origin, ray.direction, start, end, radius, distance);
#endif

        if (distance < FLT_MAX) {
            sortedAvatars.emplace_back(distance, avatar);
        }
    }

    if (sortedAvatars.size() > 1) {
        static auto comparator = [](const SortedAvatar& left, const SortedAvatar& right) { return left.first < right.first; };
        std::sort(sortedAvatars.begin(), sortedAvatars.end(), comparator);
    }

    for (auto it = sortedAvatars.begin(); it != sortedAvatars.end(); ++it) {
        const SortedAvatar& sortedAvatar = *it;
        // We can exit once avatarCapsuleDistance > bestDistance
        if (sortedAvatar.first > result.distance) {
            break;
        }

        float distance = FLT_MAX;
        BoxFace face;
        glm::vec3 surfaceNormal;
        QVariantMap extraInfo;
        SkeletonModelPointer avatarModel = sortedAvatar.second->getSkeletonModel();
        if (avatarModel->findRayIntersectionAgainstSubMeshes(ray.origin, ray.direction, distance, face, surfaceNormal, extraInfo, true)) {
            if (distance < result.distance) {
                result.intersects = true;
                result.avatarID = sortedAvatar.second->getID();
                result.distance = distance;
                result.face = face;
                result.surfaceNormal = surfaceNormal;
                result.extraInfo = extraInfo;
            }
        }
    }

    if (result.intersects) {
        result.intersection = ray.origin + ray.direction * result.distance;
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

    // It's better to intersect the ray against the avatar's actual mesh, but this is currently difficult to
    // do, because the transformed mesh data only exists over in GPU-land.  As a compromise, this code
    // intersects against the avatars capsule and then against the (T-pose) mesh.  The end effect is that picking
    // against the avatar is sort-of right, but you likely wont be able to pick against the arms.

    // TODO -- find a way to extract transformed avatar mesh data from the rendering engine.

    std::vector<SortedAvatar> sortedAvatars;
    auto avatarHashCopy = getHashCopy();
    for (auto avatarData : avatarHashCopy) {
        auto avatar = std::static_pointer_cast<Avatar>(avatarData);
        if ((avatarsToInclude.size() > 0 && !avatarsToInclude.contains(avatar->getID())) ||
            (avatarsToDiscard.size() > 0 && avatarsToDiscard.contains(avatar->getID()))) {
            continue;
        }

        float distance = FLT_MAX;
#if 0
        // if we weren't picking against the capsule, we would want to pick against the avatarBounds...
        SkeletonModelPointer avatarModel = avatar->getSkeletonModel();
        AABox avatarBounds = avatarModel->getRenderableMeshBound();
        if (avatarBounds.contains(pick.origin)) {
            distance = 0.0f;
        } else {
            float boundDistance = FLT_MAX;
            BoxFace face;
            glm::vec3 surfaceNormal;
            if (avatarBounds.findParabolaIntersection(pick.origin, pick.velocity, pick.acceleration, boundDistance, face, surfaceNormal)) {
                distance = boundDistance;
            }
        }
#else
        glm::vec3 start;
        glm::vec3 end;
        float radius;
        avatar->getCapsule(start, end, radius);
        findParabolaCapsuleIntersection(pick.origin, pick.velocity, pick.acceleration, start, end, radius, avatar->getWorldOrientation(), distance);
#endif

        if (distance < FLT_MAX) {
            sortedAvatars.emplace_back(distance, avatar);
        }
    }

    if (sortedAvatars.size() > 1) {
        static auto comparator = [](const SortedAvatar& left, const SortedAvatar& right) { return left.first < right.first; };
        std::sort(sortedAvatars.begin(), sortedAvatars.end(), comparator);
    }

    for (auto it = sortedAvatars.begin(); it != sortedAvatars.end(); ++it) {
        const SortedAvatar& sortedAvatar = *it;
        // We can exit once avatarCapsuleDistance > bestDistance
        if (sortedAvatar.first > result.parabolicDistance) {
            break;
        }

        float parabolicDistance = FLT_MAX;
        BoxFace face;
        glm::vec3 surfaceNormal;
        QVariantMap extraInfo;
        SkeletonModelPointer avatarModel = sortedAvatar.second->getSkeletonModel();
        if (avatarModel->findParabolaIntersectionAgainstSubMeshes(pick.origin, pick.velocity, pick.acceleration, parabolicDistance, face, surfaceNormal, extraInfo, true)) {
            if (parabolicDistance < result.parabolicDistance) {
                result.intersects = true;
                result.avatarID = sortedAvatar.second->getID();
                result.parabolicDistance = parabolicDistance;
                result.face = face;
                result.surfaceNormal = surfaceNormal;
                result.extraInfo = extraInfo;
            }
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

 QVariantMap AvatarManager::getAvatarTransitData() {
     QVariantMap result;
     result["frameCount"] = _transitConfig._totalFrames;
     result["framesPerMeter"] = _transitConfig._framesPerMeter;
     result["isDistanceBased"] = _transitConfig._isDistanceBased;
     result["triggerDistance"] = _transitConfig._triggerDistance;
     result["playAnimation"] = _transitConfig._playAnimation;
     result["easeType"] = (int)_transitConfig._easeType;
     result["showEffect"] = _transitConfig._showEffect;

     return result;
}
 void AvatarManager::setAvatarTransitData(const QVariantMap& data) {
     if (data.contains("frameCount")) {
         _transitConfig._totalFrames = data["frameCount"].toInt();
     }
     if (data.contains("framesPerMeter")) {
         _transitConfig._framesPerMeter = data["framesPerMeter"].toInt();
     }     
     if (data.contains("isDistanceBased")) {
         _transitConfig._isDistanceBased = data["isDistanceBased"].toBool();
     }     
     if (data.contains("triggerDistance")) {
         _transitConfig._triggerDistance = data["triggerDistance"].toDouble();
     }     
     if (data.contains("playAnimation")) {
         _transitConfig._playAnimation = data["playAnimation"].toBool();
     }
     if (data.contains("easeType")) {
         _transitConfig._easeType = (AvatarTransit::EaseType)data["easeType"].toInt();
     }
     if (data.contains("showEffect")) {
         _transitConfig._showEffect = data["showEffect"].toBool();
     }
}

 AvatarTransit::TransitAnimation AvatarManager::getAnimationFromJsonObject(const QJsonObject& object) {
     QString animationUrl;
     int firstFrame = 0;
     int frameCount = 0;
     if (object.contains("animationUrl")) {
         animationUrl = object["animationUrl"].toString();
     }
     if (object.contains("firstFrame")) {
         firstFrame = object["firstFrame"].toInt();
     }
     if (object.contains("frameCount")) {
         frameCount = object["frameCount"].toInt();
     }
     return AvatarTransit::TransitAnimation(animationUrl, firstFrame, frameCount);
 }

 void AvatarManager::setAvatarTransitAnimationData(const QVariantMap& data) {
     AvatarTransit::TransitAnimation startAnimation, middleAnimation, endAnimation;

     if (data.contains("startAnimation")) {
         QJsonObject object = data["startAnimation"].toJsonObject();
         _transitConfig._startTransitAnimation = getAnimationFromJsonObject(object);
     }
     if (data.contains("middleAnimation")) {
         QJsonObject object = data["middleAnimation"].toJsonObject();
         _transitConfig._middleTransitAnimation = getAnimationFromJsonObject(object);
     }
     if (data.contains("endAnimation")) {
         QJsonObject object = data["endAnimation"].toJsonObject();
         _transitConfig._endTransitAnimation = getAnimationFromJsonObject(object);
     }
}

 void AvatarManager::changeAvatarTransitState(AvatarTransit::Status status, const QUuid& avatarID, QUuid& effectID) {
     
     if (!_transitConfig._showEffect) {
         return;
     }
     EntityItemProperties props = getAvatarTransitEffectProperties();
     props.setParentID(avatarID);
     
     switch (status) {
     case AvatarTransit::Status::IDLE:
     case AvatarTransit::Status::TRANSITING:
     case AvatarTransit::Status::START_FRAME:
     case AvatarTransit::Status::END_FRAME:
         break;
     case AvatarTransit::Status::START_TRANSIT:
         effectID = DependencyManager::get<EntityScriptingInterface>()->addEntity(props);
         break;
     case AvatarTransit::Status::END_TRANSIT:
         props = DependencyManager::get<EntityScriptingInterface>()->getEntityProperties(effectID);
         props.setIsEmitting(false);
         auto nid = DependencyManager::get<EntityScriptingInterface>()->editEntity(effectID, props);
         break;
     }
 }

 EntityItemProperties AvatarManager::getAvatarTransitEffectProperties() {
     EntityItemProperties props;
     
     props.setIsEmitting(true);
     props.setLifetime(5.0f);
     props.setLifespan(2.7f);
     props.setMaxParticles(200.0f);
     props.setEmitRate(200.0);
     props.setEmitSpeed(2.8f);
     props.setSpeedSpread(0.2f);
     props.setLocalPosition(glm::vec3(0.0f, -1.0f, 0.0f));
     props.setEmitDimensions(glm::vec3(0.0f, 0.0f, 0.0f));
     props.setEmitOrientation(Quaternions::X_180);
     props.setEmitterShouldTrail(true);
     props.setParticleRadius(0.24f);
     props.setRadiusSpread(0.0f);
     props.setRadiusStart(0.0f);
     props.setRadiusFinish(0.21f);
     props.setColor(xColor(0.0f, 180.0f, 239.0f));
     props.setColorStart(glm::vec3(200.0f, 200.0f, 200.0f));
     props.setColorFinish(glm::vec3(0.0f, 0.0f, 0.0f));
     props.setEmitAcceleration(glm::vec3(0.0f, 0.0f, 0.0f));
     props.setAlpha(0.15f);
     props.setAccelerationSpread(glm::vec3(0.5f, 1.0f, 0.5f));
     props.setAlphaSpread(0.0f);
     props.setAlphaStart(0.3f);
     props.setAlphaFinish(0.0f);
     props.setParticleSpin(4.78f);
     props.setParentJointIndex(_myAvatar->getJointIndex("neck"));
     props.setType(EntityTypes::ParticleEffect);
     props.setTextures("http://hifi-content.s3.amazonaws.com/alexia/Particles/spark.png");
     props.setSpinSpread(0.0f);
     props.setSpinStart(-4.57f);
     props.setSpinFinish(4.67f);
     props.setRotateWithEntity(false);
     props.setPolarStart(0.0f);
     props.setPolarFinish(0.0f);
     props.setAzimuthStart(-3.1415f);
     props.setAzimuthFinish(3.1415f);
     props.setRotateWithEntity(true);
     props.setUserData("{\"grabbableKey\":{\"grabbable\":false}}");
     props.setParentJointIndex(_myAvatar->getJointIndex("head"));
     /*
     props.setIsEmitting(true);
     props.setLifespan(0.5);
     props.setMaxParticles(50);
     props.setEmitRate(50);
     props.setEmitSpeed(0.1f);
     props.setSpeedSpread(0.1f);
     props.setEmitDimensions(glm::vec3(0, 0, 0));
     props.setEmitOrientation(Quaternions::IDENTITY);
     props.setEmitterShouldTrail(true);
     props.setParticleRadius(0.05f);
     props.setRadiusSpread(0);
     props.setRadiusStart(0);
     props.setRadiusFinish(0.1);
     props.setColor(xColor(0, 180, 239));
     props.setColorStart(glm::vec3(200, 200, 200));
     props.setColorFinish(glm::vec3(0, 0, 0));
     props.setEmitAcceleration(glm::vec3(0, 0, 0));
     props.setAlpha(0.3);
     props.setAccelerationSpread(glm::vec3(0.5, 1, 0.5));
     props.setAlphaSpread(0.3);
     props.setAlphaStart(1);
     props.setAlphaFinish(0);
     props.setParticleSpin(3.3);
     props.setParentJointIndex(_myAvatar->getJointIndex("head"));
     props.setType(EntityTypes::ParticleEffect);
     props.setTextures("http://hifi-content.s3.amazonaws.com/alexia/Particles/circle.png");
     props.setSpinSpread(2.53);
     props.setSpinStart(3.31);
     props.setSpinFinish(3.31);
     props.setRotateWithEntity(false);
     props.setPolarStart(0);
     props.setPolarFinish(3.1415);
     props.setAzimuthStart(-3.1415);
     props.setAzimuthFinish(3.1415);
     props.setUserData("{\"grabbableKey\":{\"grabbable\":false}}");
     */
     return props;
 }