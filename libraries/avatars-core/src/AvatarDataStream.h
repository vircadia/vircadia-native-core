//
//  AvatarDataStream.h
//  libraries/avatars-core/src
//
//  Created by Nshan G. on 21 Apr 2022.
//  Copyright 2013 High Fidelity, Inc.
//  Copyright 2022 Vircadia contributors.
//  Copyright 2022 DigiSomni LLC.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef LIBRARIES_AVATARS_CORE_SRC_AVATARDATASTREAM_H
#define LIBRARIES_AVATARS_CORE_SRC_AVATARDATASTREAM_H

#include <string>
#include <memory>
#include <queue>
#include <inttypes.h>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QByteArray>
#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QRect>
#include <QStringList>
#include <QUrl>
#include <QUuid>
#include <QVariantMap>
#include <QVector>
#include <QtScript/QScriptable>
#include <QtScript/QScriptValueIterator>
#include <QReadWriteLock>

#include <AvatarConstants.h>
#include <JointData.h>
#include <NLPacket.h>
#include <Node.h>
#include <NumericalConstants.h>
#include <Packed.h>
#include <SharedUtil.h>
#include <SimpleMovingAverage.h>
#include <ThreadSafeValueCache.h>
#include <shared/RateCounter.h>
#include <shared/ReadWriteLockable.h>
#include <udt/SequenceNumber.h>
#include <Grab.h>

#include "PathUtils.h"

#include "AvatarTraits.h"
#include "ClientTraitsHandler.h"

using AvatarEntityMap = QMap<QUuid, QByteArray>;
using PackedAvatarEntityMap = QMap<QUuid, QByteArray>; // similar to AvatarEntityMap, but different internal format
using AvatarEntityIDs = QSet<QUuid>;

using AvatarGrabDataMap = QMap<QUuid, QByteArray>;

using AvatarDataSequenceNumber = uint16_t;

const int MAX_NUM_AVATAR_ENTITIES = 42;

// avatar motion behaviors
const quint32 AVATAR_MOTION_ACTION_MOTOR_ENABLED = 1U << 0;
const quint32 AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED = 1U << 1;

const quint32 AVATAR_MOTION_DEFAULTS =
        AVATAR_MOTION_ACTION_MOTOR_ENABLED |
        AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED;

// these bits will be expanded as features are exposed
const quint32 AVATAR_MOTION_SCRIPTABLE_BITS =
        AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED;

// Bitset of state flags - we store the key state, hand state, Faceshift, eye tracking, and existence of
// referential data in this bit set. The hand state is an octal, but is split into two sections to maintain
// backward compatibility. The bits are ordered as such (0-7 left to right).
// AA 6/1/18 added three more flags bits 8,9, and 10 for procedural audio, blink, and eye saccade enabled
//
//     +-----+-----+-+-+-+--+--+--+--+-----+
//     |K0,K1|H0,H1|F|E|R|H2|Au|Bl|Ey|xxxxx|
//     +-----+-----+-+-+-+--+--+--+--+-----+
//
// Key state - K0,K1 is found in the 1st and 2nd bits
// Hand state - H0,H1,H2 is found in the 3rd, 4th, and 8th bits
// Face tracker - F is found in the 5th bit
// Eye tracker - E is found in the 6th bit
// Referential Data - R is found in the 7th bit
// Procedural audio to mouth movement is enabled 8th bit
// Procedural Blink is enabled 9th bit
// Procedural Eyelid is enabled 10th bit
// Procedural PROCEDURAL_BLINK_FACE_MOVEMENT is enabled 11th bit
// Procedural Collide with other avatars is enabled 12th bit
// Procedural Has Hero Priority is enabled 13th bit

const int KEY_STATE_START_BIT = 0; // 1st and 2nd bits  (UNUSED)
const int HAND_STATE_START_BIT = 2; // 3rd and 4th bits (UNUSED)
const int HAS_SCRIPTED_BLENDSHAPES = 4; // 5th bit
const int HAS_PROCEDURAL_EYE_MOVEMENT = 5; // 6th bit
const int HAS_REFERENTIAL = 6; // 7th bit
const int HAND_STATE_FINGER_POINTING_BIT = 7; // 8th bit (UNUSED)
const int AUDIO_ENABLED_FACE_MOVEMENT = 8; // 9th bit
const int PROCEDURAL_EYE_FACE_MOVEMENT = 9; // 10th bit
const int PROCEDURAL_BLINK_FACE_MOVEMENT = 10; // 11th bit
const int COLLIDE_WITH_OTHER_AVATARS = 11; // 12th bit
const int HAS_HERO_PRIORITY = 12; // 13th bit  (be scared)

/*@jsdoc
 * <p>The pointing state of the hands is specified by the following values:</p>
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code></td><td>No hand is pointing.</td></tr>
 *     <tr><td><code>1</code></td><td>The left hand is pointing.</td></tr>
 *     <tr><td><code>2</code></td><td>The right hand is pointing.</td></tr>
 *     <tr><td><code>4</code></td><td>It is the index finger that is pointing.</td></tr>
 *   </tbody>
 * </table>
 * <p>The values for the hand states are added together to give the <code>HandState</code> value. For example, if the left
 * hand's finger is pointing, the value is <code>1 + 4 == 5</code>.
 * @typedef {number} HandState
 */
const char HAND_STATE_NULL = 0;
const char LEFT_HAND_POINTING_FLAG = 1;
const char RIGHT_HAND_POINTING_FLAG = 2;
const char IS_FINGER_POINTING_FLAG = 4;

// AvatarData state flags - we store the details about the packet encoding in the first byte,
// before the "header" structure
const char AVATARDATA_FLAGS_MINIMUM = 0;

using SmallFloat = uint16_t; // a compressed float with less precision, user defined radix

namespace AvatarSkeletonTrait {
    enum BoneType : uint8_t {
        SkeletonRoot = 0,
        SkeletonChild,
        NonSkeletonRoot,
        NonSkeletonChild
    };

    PACKED_BEGIN struct Header {
        float maxTranslationDimension;
        float maxScaleDimension;
        uint8_t numJoints;
        uint16_t stringTableLength;
    } PACKED_END;

    PACKED_BEGIN struct JointData {
        uint16_t stringStart;
        uint8_t stringLength;
        uint8_t boneType;
        uint8_t defaultTranslation[6];
        uint8_t defaultRotation[6];
        uint16_t defaultScale;
        uint16_t jointIndex;
        uint16_t parentIndex;
    } PACKED_END;

    struct UnpackedJointData {
        int stringStart;
        int stringLength;
        int boneType;
        glm::vec3 defaultTranslation;
        glm::quat defaultRotation;
        float defaultScale;
        int jointIndex;
        int parentIndex;
        QString jointName;
    };

    inline bool operator==(const AvatarSkeletonTrait::UnpackedJointData& a, const AvatarSkeletonTrait::UnpackedJointData& b) {
        return a.stringStart == b.stringStart &&
            a.stringLength == b.stringLength &&
            a.defaultTranslation == b.defaultTranslation &&
            a.defaultRotation == b.defaultRotation &&
            a.defaultScale == b.defaultScale &&
            a.jointIndex == b.jointIndex &&
            a.parentIndex == b.parentIndex &&
            a.jointName == b.jointName &&
            a.boneType == b.boneType;
    }
    inline bool operator!=(const AvatarSkeletonTrait::UnpackedJointData& a, const AvatarSkeletonTrait::UnpackedJointData& b) {
        return !(a == b);
    }
}

class AttachmentData;

namespace AvatarDataPacket {

    // NOTE: every time avatar data is sent from mixer to client, it also includes the GUIID for the session
    // this is 16bytes of data at 45hz that's 5.76kbps
    // it might be nice to use a dictionary to compress that

    // Packet State Flags - we store the details about the existence of other records in this bitset:
    // AvatarGlobalPosition, Avatar face tracker, eye tracking, and existence of
    using HasFlags = uint16_t;
    const HasFlags PACKET_HAS_AVATAR_GLOBAL_POSITION   = 1U << 0;
    const HasFlags PACKET_HAS_AVATAR_BOUNDING_BOX      = 1U << 1;
    const HasFlags PACKET_HAS_AVATAR_ORIENTATION       = 1U << 2;
    const HasFlags PACKET_HAS_AVATAR_SCALE             = 1U << 3;
    const HasFlags PACKET_HAS_LOOK_AT_POSITION         = 1U << 4;
    const HasFlags PACKET_HAS_AUDIO_LOUDNESS           = 1U << 5;
    const HasFlags PACKET_HAS_SENSOR_TO_WORLD_MATRIX   = 1U << 6;
    const HasFlags PACKET_HAS_ADDITIONAL_FLAGS         = 1U << 7;
    const HasFlags PACKET_HAS_PARENT_INFO              = 1U << 8;
    const HasFlags PACKET_HAS_AVATAR_LOCAL_POSITION    = 1U << 9;
    const HasFlags PACKET_HAS_HAND_CONTROLLERS         = 1U << 10;
    const HasFlags PACKET_HAS_FACE_TRACKER_INFO        = 1U << 11;
    const HasFlags PACKET_HAS_JOINT_DATA               = 1U << 12;
    const HasFlags PACKET_HAS_JOINT_DEFAULT_POSE_FLAGS = 1U << 13;
    const HasFlags PACKET_HAS_GRAB_JOINTS              = 1U << 14;
    const HasFlags ALL_HAS_FLAGS                       = (1U << 15) - 1;
    const size_t AVATAR_HAS_FLAGS_SIZE = 2;

    using SixByteQuat = uint8_t[6];
    using SixByteTrans = uint8_t[6];

    // NOTE: AvatarDataPackets start with a uint16_t sequence number that is not reflected in the Header structure.

    PACKED_BEGIN struct Header {
        HasFlags packetHasFlags;        // state flags, indicated which additional records are included in the packet
    } PACKED_END;
    const size_t HEADER_SIZE = 2;
    static_assert(sizeof(Header) == HEADER_SIZE, "AvatarDataPacket::Header size doesn't match.");

    PACKED_BEGIN struct Vec3 {
        float data[3];          // vector coordinates
    } PACKED_END;

    PACKED_BEGIN struct Quat {
        float data[4];          // quaternion components
    } PACKED_END;

    PACKED_BEGIN struct AvatarGlobalPosition {
        Vec3 globalPosition;          // avatar's position
    } PACKED_END;
    const size_t AVATAR_GLOBAL_POSITION_SIZE = 12;
    static_assert(sizeof(AvatarGlobalPosition) == AVATAR_GLOBAL_POSITION_SIZE, "AvatarDataPacket::AvatarGlobalPosition size doesn't match.");

    PACKED_BEGIN struct AvatarBoundingBox {
        Vec3 avatarDimensions;        // avatar's bounding box in world space units, but relative to the position.
        Vec3 boundOriginOffset;       // offset from the position of the avatar to the origin of the bounding box
    } PACKED_END;
    const size_t AVATAR_BOUNDING_BOX_SIZE = 24;
    static_assert(sizeof(AvatarBoundingBox) == AVATAR_BOUNDING_BOX_SIZE, "AvatarDataPacket::AvatarBoundingBox size doesn't match.");

    PACKED_BEGIN struct AvatarOrientation {
        SixByteQuat avatarOrientation;      // encodeded and compressed by packOrientationQuatToSixBytes()
    } PACKED_END;
    const size_t AVATAR_ORIENTATION_SIZE = 6;
    static_assert(sizeof(AvatarOrientation) == AVATAR_ORIENTATION_SIZE, "AvatarDataPacket::AvatarOrientation size doesn't match.");

    PACKED_BEGIN struct AvatarScale {
        SmallFloat scale;                 // avatar's scale, compressed by packFloatRatioToTwoByte()
    } PACKED_END;
    const size_t AVATAR_SCALE_SIZE = 2;
    static_assert(sizeof(AvatarScale) == AVATAR_SCALE_SIZE, "AvatarDataPacket::AvatarScale size doesn't match.");

    PACKED_BEGIN struct LookAtPosition {
        Vec3 lookAtPosition;              // world space position that eyes are focusing on.
                                          // FIXME - unless the person has an eye tracker, this is simulated...
                                          //    a) maybe we can just have the client calculate this
                                          //    b) at distance this will be hard to discern and can likely be
                                          //       descimated or dropped completely
                                          //
                                          // POTENTIAL SAVINGS - 12 bytes
    } PACKED_END;
    const size_t LOOK_AT_POSITION_SIZE = 12;
    static_assert(sizeof(LookAtPosition) == LOOK_AT_POSITION_SIZE, "AvatarDataPacket::LookAtPosition size doesn't match.");

    PACKED_BEGIN struct AudioLoudness {
        uint8_t audioLoudness;            // current loudness of microphone compressed with packFloatGainToByte()
    } PACKED_END;
    const size_t AUDIO_LOUDNESS_SIZE = 1;
    static_assert(sizeof(AudioLoudness) == AUDIO_LOUDNESS_SIZE, "AvatarDataPacket::AudioLoudness size doesn't match.");

    PACKED_BEGIN struct SensorToWorldMatrix {
        // FIXME - these 20 bytes are only used by viewers if my avatar has "attachments"
        // we could save these bytes if no attachments are active.
        //
        // POTENTIAL SAVINGS - 20 bytes

        SixByteQuat sensorToWorldQuat;    // 6 byte compressed quaternion part of sensor to world matrix
        uint16_t sensorToWorldScale;      // uniform scale of sensor to world matrix
        float sensorToWorldTrans[3];      // fourth column of sensor to world matrix
                                          // FIXME - sensorToWorldTrans might be able to be better compressed if it was
                                          // relative to the avatar position.
    } PACKED_END;
    const size_t SENSOR_TO_WORLD_SIZE = 20;
    static_assert(sizeof(SensorToWorldMatrix) == SENSOR_TO_WORLD_SIZE, "AvatarDataPacket::SensorToWorldMatrix size doesn't match.");

    PACKED_BEGIN struct AdditionalFlags {
        uint16_t flags;                    // additional flags: hand state, key state, eye tracking
    } PACKED_END;
    const size_t ADDITIONAL_FLAGS_SIZE = 2;
    static_assert(sizeof(AdditionalFlags) == ADDITIONAL_FLAGS_SIZE, "AvatarDataPacket::AdditionalFlags size doesn't match.");

    // only present if HAS_REFERENTIAL flag is set in AvatarInfo.flags
    PACKED_BEGIN struct ParentInfo {
        uint8_t parentUUID[NUM_BYTES_RFC4122_UUID ];       // rfc 4122 encoded
        uint16_t parentJointIndex;
    } PACKED_END;
    const size_t PARENT_INFO_SIZE = 18;
    static_assert(sizeof(ParentInfo) == PARENT_INFO_SIZE, "AvatarDataPacket::ParentInfo size doesn't match.");

    // will only ever be included if the avatar has a parent but can change independent of changes to parent info
    // and so we keep it a separate record
    PACKED_BEGIN struct AvatarLocalPosition {
        Vec3 localPosition;           // parent frame translation of the avatar
    } PACKED_END;

    const size_t AVATAR_LOCAL_POSITION_SIZE = 12;
    static_assert(sizeof(AvatarLocalPosition) == AVATAR_LOCAL_POSITION_SIZE, "AvatarDataPacket::AvatarLocalPosition size doesn't match.");

    PACKED_BEGIN struct HandControllers {
        SixByteQuat leftHandRotation;
        SixByteTrans leftHandTranslation;
        SixByteQuat rightHandRotation;
        SixByteTrans rightHandTranslation;
    } PACKED_END;
    static const size_t HAND_CONTROLLERS_SIZE = 24;
    static_assert(sizeof(HandControllers) == HAND_CONTROLLERS_SIZE, "AvatarDataPacket::HandControllers size doesn't match.");

    const size_t MAX_CONSTANT_HEADER_SIZE = HEADER_SIZE +
        AVATAR_GLOBAL_POSITION_SIZE +
        AVATAR_BOUNDING_BOX_SIZE +
        AVATAR_ORIENTATION_SIZE +
        AVATAR_SCALE_SIZE +
        LOOK_AT_POSITION_SIZE +
        AUDIO_LOUDNESS_SIZE +
        SENSOR_TO_WORLD_SIZE +
        ADDITIONAL_FLAGS_SIZE +
        PARENT_INFO_SIZE +
        AVATAR_LOCAL_POSITION_SIZE +
        HAND_CONTROLLERS_SIZE;

    // variable length structure follows

    // only present if HAS_SCRIPTED_BLENDSHAPES flag is set in AvatarInfo.flags
    PACKED_BEGIN struct FaceTrackerInfo {
        float leftEyeBlink;
        float rightEyeBlink;
        float averageLoudness;
        float browAudioLift;
        uint8_t numBlendshapeCoefficients;
        // float blendshapeCoefficients[numBlendshapeCoefficients];
    } PACKED_END;
    const size_t FACE_TRACKER_INFO_SIZE = 17;
    static_assert(sizeof(FaceTrackerInfo) == FACE_TRACKER_INFO_SIZE, "AvatarDataPacket::FaceTrackerInfo size doesn't match.");
    size_t maxFaceTrackerInfoSize(size_t numBlendshapeCoefficients);

    /*
    struct JointData {
        uint8_t numJoints;
        uint8_t rotationValidityBits[ceil(numJoints / 8)];     // one bit per joint, if true then a compressed rotation follows.
        SixByteQuat rotation[numValidRotations];               // encodeded and compressed by packOrientationQuatToSixBytes()
        uint8_t translationValidityBits[ceil(numJoints / 8)];  // one bit per joint, if true then a compressed translation follows.
        float maxTranslationDimension;                         // used to normalize fixed point translation values.
        SixByteTrans translation[numValidTranslations];        // normalized and compressed by packFloatVec3ToSignedTwoByteFixed()
        SixByteQuat leftHandControllerRotation;
        SixByteTrans leftHandControllerTranslation;
        SixByteQuat rightHandControllerRotation;
        SixByteTrans rightHandControllerTranslation;
    };
    */
    size_t maxJointDataSize(size_t numJoints);
    size_t minJointDataSize(size_t numJoints);

    /*
    struct JointDefaultPoseFlags {
       uint8_t numJoints;
       uint8_t rotationIsDefaultPoseBits[ceil(numJoints / 8)];
       uint8_t translationIsDefaultPoseBits[ceil(numJoints / 8)];
    };
    */
    size_t maxJointDefaultPoseFlagsSize(size_t numJoints);

    PACKED_BEGIN struct FarGrabJoints {
        Vec3 leftFarGrabPosition; // left controller far-grab joint position
        Quat leftFarGrabRotation; // left controller far-grab joint rotation
        Vec3 rightFarGrabPosition; // right controller far-grab joint position
        Quat rightFarGrabRotation; // right controller far-grab joint rotation
        Vec3 mouseFarGrabPosition; // mouse far-grab joint position
        Quat mouseFarGrabRotation; // mouse far-grab joint rotation
    } PACKED_END;
    const size_t FAR_GRAB_JOINTS_SIZE = 84;
    static_assert(sizeof(FarGrabJoints) == FAR_GRAB_JOINTS_SIZE, "AvatarDataPacket::FarGrabJoints size doesn't match.");

    static const size_t MIN_BULK_PACKET_SIZE = NUM_BYTES_RFC4122_UUID + HEADER_SIZE;

    // AvatarIdentity packet:
    enum class IdentityFlag: quint32 {none, isReplicated = 0x1, lookAtSnapping = 0x2, verificationFailed = 0x4};
    Q_DECLARE_FLAGS(IdentityFlags, IdentityFlag)

    struct Identity {
        QVector<AttachmentData> attachmentData;
        QString displayName;
        QString sessionDisplayName;
        IdentityFlags identityFlags;

        bool operator==(const Identity& other) const {
            return attachmentData == other.attachmentData &&
                displayName == other.displayName &&
                sessionDisplayName == other.sessionDisplayName &&
                identityFlags == other.identityFlags;
        }
        bool operator!=(const Identity& other) const {
            return !(*this == other);
        }
    };

    struct SendStatus {
        HasFlags itemFlags { 0 };
        bool sendUUID { false };
        int rotationsSent { 0 };  // ie: index of next unsent joint
        int translationsSent { 0 };
        operator bool() { return itemFlags == 0; }
    };
}

const float MAX_AUDIO_LOUDNESS = 1000.0f; // close enough for mouth animation

const QString DEFAULT_FULL_AVATAR_MODEL_NAME = QString("Default");

// how often should we send a full report about joint rotations, even if they haven't changed?
const float AVATAR_SEND_FULL_UPDATE_RATIO = 0.02f;
// this controls how large a change in joint-rotation must be before the interface sends it to the avatar mixer
const float AVATAR_MIN_ROTATION_DOT = 0.9999999f;
const float AVATAR_MIN_TRANSLATION = 0.0001f;

// quaternion dot products
const float ROTATION_CHANGE_2D = 0.99984770f; // 2 degrees
const float ROTATION_CHANGE_4D = 0.99939083f; // 4 degrees
const float ROTATION_CHANGE_6D = 0.99862953f; // 6 degrees
const float ROTATION_CHANGE_15D = 0.99144486f; // 15 degrees
const float ROTATION_CHANGE_179D = 0.00872653f; // 179 degrees

// rotation culling distance thresholds
const float AVATAR_DISTANCE_LEVEL_1 = 12.5f; // meters
const float AVATAR_DISTANCE_LEVEL_2 = 16.6f; // meters
const float AVATAR_DISTANCE_LEVEL_3 = 25.0f; // meters
const float AVATAR_DISTANCE_LEVEL_4 = 50.0f; // meters
const float AVATAR_DISTANCE_LEVEL_5 = 200.0f; // meters

// Where one's own Avatar begins in the world (will be overwritten if avatar data file is found).
// This is the start location in the Sandbox (xyz: 6270, 211, 6000).
const glm::vec3 START_LOCATION(6270, 211, 6000);

// Avatar Transit Constants
const float AVATAR_TRANSIT_MIN_TRIGGER_DISTANCE = 1.0f;
const float AVATAR_TRANSIT_MAX_TRIGGER_DISTANCE = 30.0f;
const int AVATAR_TRANSIT_FRAME_COUNT = 5;
const float AVATAR_TRANSIT_FRAMES_PER_METER = 0.5f;
const float AVATAR_TRANSIT_ABORT_DISTANCE = 0.1f;
const bool AVATAR_TRANSIT_DISTANCE_BASED = false;
const float AVATAR_TRANSIT_FRAMES_PER_SECOND = 30.0f;
const float AVATAR_PRE_TRANSIT_FRAME_COUNT = 10.0f;
const float AVATAR_POST_TRANSIT_FRAME_COUNT = 27.0f;

enum KeyState : uint8_t {
    NO_KEY_DOWN = 0,
    INSERT_KEY_DOWN,
    DELETE_KEY_DOWN
};

enum KillAvatarReason : uint8_t {
    NoReason = 0,
    AvatarDisconnected,
    AvatarIgnored,
    TheirAvatarEnteredYourBubble,
    YourAvatarEnteredTheirBubble
};

Q_DECLARE_METATYPE(KillAvatarReason);

class QDataStream;

class AvatarDataRate {
public:
    RateCounter<> globalPositionRate;
    RateCounter<> localPositionRate;
    RateCounter<> handControllersRate;
    RateCounter<> avatarBoundingBoxRate;
    RateCounter<> avatarOrientationRate;
    RateCounter<> avatarScaleRate;
    RateCounter<> lookAtPositionRate;
    RateCounter<> audioLoudnessRate;
    RateCounter<> sensorToWorldRate;
    RateCounter<> additionalFlagsRate;
    RateCounter<> parentInfoRate;
    RateCounter<> faceTrackerRate;
    RateCounter<> jointDataRate;
    RateCounter<> jointDefaultPoseFlagsRate;
    RateCounter<> farGrabJointRate;
};


template <typename Derived>
class AvatarDataStream {


public:

    AvatarDataStream();
    ~AvatarDataStream();

    typedef enum {
        NoData,
        PALMinimum,
        MinimumData,
        CullSmallData,
        IncludeSmallData,
        SendAllData
    } AvatarDataDetail;

    // FIXME: CRTP move to derived
    // virtual QByteArray toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking = false);
    // virtual QByteArray toByteArray(AvatarDataDetail dataDetail, quint64 lastSentTime, const QVector<JointData>& lastSentJointData,
    //     AvatarDataPacket::SendStatus& sendStatus, bool dropFaceTracking, bool distanceAdjust, glm::vec3 viewerPosition,
    //     QVector<JointData>* sentJointDataOut, int maxDataSize = 0, AvatarDataRate* outboundDataRateOut = nullptr) const;
    QByteArray toByteArray(AvatarDataPacket::HasFlags itemFlagsInit, AvatarDataDetail dataDetail,
           const QVector<JointData>& lastSentJointData, AvatarDataPacket::SendStatus& sendStatus,
           bool distanceAdjust, glm::vec3 viewerPosition,
           QVector<JointData>* sentJointDataOut,
           int maxDataSize, AvatarDataRate* outboundDataRateOut) const;

    void doneEncoding(bool cullSmallChanges, const AvatarDataPacket::SendStatus& sendStatus);
    void doneEncoding(bool cullSmallChanges);

    int parseDataFromBuffer(const QByteArray& buffer);

    int getJointIndex(const QString& name) const;

    void storeAvatarEntityDataPayload(const QUuid& entityID, const QByteArray& payload);

    void updateAvatarEntity(const QUuid& entityID, const QByteArray& entityData);

    void clearAvatarEntity(const QUuid& entityID, bool requiresRemovalFromTree = true);

    void clearAvatarEntityInternal(const QUuid& entityID);

    void clearAvatarEntities();

    QList<QUuid> getAvatarEntityIDs() const;

    // identityChanged returns true if identity has changed, false otherwise. Similarly for displayNameChanged and skeletonModelUrlChange.
    void processAvatarIdentity(QDataStream& packetStream, bool& identityChanged, bool& displayNameChanged);

    QByteArray packTrait(AvatarTraits::TraitType traitType) const;
    QByteArray packTraitInstance(AvatarTraits::TraitType traitType, AvatarTraits::TraitInstanceID instanceID);

    void processTrait(AvatarTraits::TraitType traitType, QByteArray traitBinaryData);
    void processTraitInstance(AvatarTraits::TraitType traitType,
                              AvatarTraits::TraitInstanceID instanceID, QByteArray traitBinaryData);
    void processDeletedTraitInstance(AvatarTraits::TraitType traitType, AvatarTraits::TraitInstanceID instanceID);

    void prepareResetTraitInstances();

    QByteArray identityByteArray(bool setIsReplicated = false) const;

    void sendSkeletonModelURL();

    int getAverageBytesReceivedPerSecond() const;
    int getReceiveRate() const;

    void setAvatarEntityDataChanged(bool value) { _avatarEntityDataChanged = value; }
    AvatarEntityIDs getAndClearRecentlyRemovedIDs();

    float getDataRate(const QString& rateName = QString("")) const;

    float getUpdateRate(const QString& rateName = QString("")) const;

    QVector<JointData> getLastSentJointData();

    bool hasProcessedFirstIdentity() const { return _hasProcessedFirstIdentity; }

    void setReplicaIndex(int replicaIndex) { _replicaIndex = replicaIndex; }
    int getReplicaIndex() { return _replicaIndex; }

    void setIsNewAvatar(bool isNewAvatar) { _isNewAvatar = isNewAvatar; }
    bool getIsNewAvatar() { return _isNewAvatar; }
    void setIsClientAvatar(bool isClientAvatar) { _isClientAvatar = isClientAvatar; }
    void sendSkeletonData();

    int sendAllPackets(AvatarDataDetail, AvatarDataPacket::SendStatus&);

    int sendAvatarDataPacket(AvatarDataDetail, AvatarDataPacket::SendStatus&);

    int sendAvatarDataPacket(const QByteArray&);

    /*@jsdoc
     * @function Avatar.sendAvatarDataPacket
     * @param {boolean} [sendAll=false] - Send all.
     * @returns {number}
     * @deprecated This function is deprecated and will be removed.
     */
    // FIXME: CRTP move to derived
    // virtual int sendAvatarDataPacket(bool sendAll = false);

    int sendIdentityPacket();

    QUuid grab(const QUuid& targetID, int parentJointIndex,
                           glm::vec3 positionalOffset, glm::quat rotationalOffset);
    void releaseGrab(const QUuid& grabID);

protected:
    using Clock = std::chrono::system_clock;
    using TimePoint = Clock::time_point;

    void insertRemovedEntityID(const QUuid entityID);

    float getDistanceBasedMinRotationDOT(glm::vec3 viewerPosition) const;
    float getDistanceBasedMinTranslationDistance(glm::vec3 viewerPosition) const;

    QByteArray packSkeletonData() const;
    QByteArray packSkeletonModelURL() const;
    QByteArray packAvatarEntityTraitInstance(AvatarTraits::TraitInstanceID traitInstanceID);
    QByteArray packGrabTraitInstance(AvatarTraits::TraitInstanceID traitInstanceID);

    void unpackSkeletonModelURL(const QByteArray& data);
    void unpackSkeletonData(const QByteArray& data);

    QVector<JointData> _lastSentJointData; ///< the state of the skeleton joints last time we transmitted

    bool _hasNewJointData { true }; // set in here, cleared in Avatar

    QUrl _skeletonModelURL;

    SimpleMovingAverage _averageBytesReceived;

    // Some rate data for incoming data in bytes
    RateCounter<> _parseBufferRate;
    RateCounter<> _globalPositionRate;
    RateCounter<> _localPositionRate;
    RateCounter<> _handControllersRate;
    RateCounter<> _avatarBoundingBoxRate;
    RateCounter<> _avatarOrientationRate;
    RateCounter<> _avatarScaleRate;
    RateCounter<> _lookAtPositionRate;
    RateCounter<> _audioLoudnessRate;
    RateCounter<> _sensorToWorldRate;
    RateCounter<> _additionalFlagsRate;
    RateCounter<> _parentInfoRate;
    RateCounter<> _faceTrackerRate;
    RateCounter<> _jointDataRate;
    RateCounter<> _jointDefaultPoseFlagsRate;
    RateCounter<> _farGrabJointRate;

    // Some rate data for incoming data updates
    RateCounter<> _parseBufferUpdateRate;
    RateCounter<> _globalPositionUpdateRate;
    RateCounter<> _localPositionUpdateRate;
    RateCounter<> _handControllersUpdateRate;
    RateCounter<> _avatarBoundingBoxUpdateRate;
    RateCounter<> _avatarOrientationUpdateRate;
    RateCounter<> _avatarScaleUpdateRate;
    RateCounter<> _lookAtPositionUpdateRate;
    RateCounter<> _audioLoudnessUpdateRate;
    RateCounter<> _sensorToWorldUpdateRate;
    RateCounter<> _additionalFlagsUpdateRate;
    RateCounter<> _parentInfoUpdateRate;
    RateCounter<> _faceTrackerUpdateRate;
    RateCounter<> _jointDataUpdateRate;
    RateCounter<> _jointDefaultPoseFlagsUpdateRate;
    RateCounter<> _farGrabJointUpdateRate;

    // Some rate data for outgoing data
    AvatarDataRate _outboundDataRate;

    mutable ReadWriteLockable _avatarEntitiesLock;
    AvatarEntityIDs _avatarEntityRemoved; // recently removed AvatarEntity ids
    PackedAvatarEntityMap _packedAvatarEntityData;
    bool _avatarEntityDataChanged { false };

    mutable ReadWriteLockable _avatarGrabsLock;
    AvatarGrabDataMap _avatarGrabData;
    bool _avatarGrabDataChanged { false }; // by network

    int getFauxJointIndex(const QString& name) const;

    bool _hasProcessedFirstIdentity { false };
    int _replicaIndex { 0 };
    bool _isNewAvatar { true };
    bool _isClientAvatar { false };

    bool updateAvatarGrabData(const QUuid& grabID, const QByteArray& grabData);
    void clearAvatarGrabData(const QUuid& grabID);

private:
    Derived& derived();
    const Derived& derived() const;

    TimePoint _nextTraitsSendWindow;

};

class AttachmentData {
public:
    QUrl modelURL;
    QString jointName;
    glm::vec3 translation;
    glm::quat rotation;
    float scale { 1.0f };
    bool isSoft { false };

    bool isValid() const { return modelURL.isValid(); }

    bool operator==(const AttachmentData& other) const;

    QVariant toVariant() const;
    bool fromVariant(const QVariant& variant);
};

QDataStream& operator<<(QDataStream& out, const AttachmentData& attachment);
QDataStream& operator>>(QDataStream& in, AttachmentData& attachment);

Q_DECLARE_METATYPE(AttachmentData)
Q_DECLARE_METATYPE(QVector<AttachmentData>)

struct HandControllerVantage {
    glm::vec3 position;
    glm::quat orientation;
};

// faux joint indexes (-1 means invalid)
const int NO_JOINT_INDEX = 65535; // -1
const int SENSOR_TO_WORLD_MATRIX_INDEX = 65534; // -2
const int CONTROLLER_RIGHTHAND_INDEX = 65533; // -3
const int CONTROLLER_LEFTHAND_INDEX = 65532; // -4
const int CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX = 65531; // -5
const int CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX = 65530; // -6
const int CAMERA_MATRIX_INDEX = 65529; // -7
const int FARGRAB_RIGHTHAND_INDEX = 65528; // -8
const int FARGRAB_LEFTHAND_INDEX = 65527; // -9
const int FARGRAB_MOUSE_INDEX = 65526; // -10

const int LOWEST_PSEUDO_JOINT_INDEX = 65526;

const int MAX_NUM_AVATAR_GRABS = 6;

#endif /* end of include guard */
