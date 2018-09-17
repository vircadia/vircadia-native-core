//
//  AvatarData.h
//  libraries/avatars/src
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AvatarData_h
#define hifi_AvatarData_h

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
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h>
#include <SimpleMovingAverage.h>
#include <SpatiallyNestable.h>
#include <ThreadSafeValueCache.h>
#include <ViewFrustum.h>
#include <shared/RateCounter.h>
#include <udt/SequenceNumber.h>

#include "AABox.h"
#include "AvatarTraits.h"
#include "HeadData.h"
#include "PathUtils.h"

#include <graphics/Material.h>

using AvatarSharedPointer = std::shared_ptr<AvatarData>;
using AvatarWeakPointer = std::weak_ptr<AvatarData>;
using AvatarHash = QHash<QUuid, AvatarSharedPointer>;
using AvatarEntityMap = QMap<QUuid, QByteArray>;
using AvatarEntityIDs = QSet<QUuid>;

using AvatarDataSequenceNumber = uint16_t;

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

const int KEY_STATE_START_BIT = 0; // 1st and 2nd bits
const int HAND_STATE_START_BIT = 2; // 3rd and 4th bits
const int IS_FACE_TRACKER_CONNECTED = 4; // 5th bit
const int IS_EYE_TRACKER_CONNECTED = 5; // 6th bit (was CHAT_CIRCLING)
const int HAS_REFERENTIAL = 6; // 7th bit
const int HAND_STATE_FINGER_POINTING_BIT = 7; // 8th bit
const int AUDIO_ENABLED_FACE_MOVEMENT = 8; // 9th bit
const int PROCEDURAL_EYE_FACE_MOVEMENT = 9; // 10th bit
const int PROCEDURAL_BLINK_FACE_MOVEMENT = 10; // 11th bit


const char HAND_STATE_NULL = 0;
const char LEFT_HAND_POINTING_FLAG = 1;
const char RIGHT_HAND_POINTING_FLAG = 2;
const char IS_FINGER_POINTING_FLAG = 4;

// AvatarData state flags - we store the details about the packet encoding in the first byte,
// before the "header" structure
const char AVATARDATA_FLAGS_MINIMUM = 0;

using SmallFloat = uint16_t; // a compressed float with less precision, user defined radix

namespace AvatarDataPacket {

    // NOTE: every time AvatarData is sent from mixer to client, it also includes the GUIID for the session
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
    const HasFlags PACKET_HAS_FACE_TRACKER_INFO        = 1U << 10;
    const HasFlags PACKET_HAS_JOINT_DATA               = 1U << 11;
    const HasFlags PACKET_HAS_JOINT_DEFAULT_POSE_FLAGS = 1U << 12;
    const HasFlags PACKET_HAS_GRAB_JOINTS              = 1U << 13;
    const size_t AVATAR_HAS_FLAGS_SIZE = 2;

    using SixByteQuat = uint8_t[6];
    using SixByteTrans = uint8_t[6];

    // NOTE: AvatarDataPackets start with a uint16_t sequence number that is not reflected in the Header structure.

    PACKED_BEGIN struct Header {
        HasFlags packetHasFlags;        // state flags, indicated which additional records are included in the packet
    } PACKED_END;
    const size_t HEADER_SIZE = 2;
    static_assert(sizeof(Header) == HEADER_SIZE, "AvatarDataPacket::Header size doesn't match.");

    PACKED_BEGIN struct AvatarGlobalPosition {
        float globalPosition[3];          // avatar's position
    } PACKED_END;
    const size_t AVATAR_GLOBAL_POSITION_SIZE = 12;
    static_assert(sizeof(AvatarGlobalPosition) == AVATAR_GLOBAL_POSITION_SIZE, "AvatarDataPacket::AvatarGlobalPosition size doesn't match.");

    PACKED_BEGIN struct AvatarBoundingBox {
        float avatarDimensions[3];        // avatar's bounding box in world space units, but relative to the position.
        float boundOriginOffset[3];       // offset from the position of the avatar to the origin of the bounding box
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
        float lookAtPosition[3];          // world space position that eyes are focusing on.
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

        SixByteQuat sensorToWorldQuat;     // 6 byte compressed quaternion part of sensor to world matrix
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
        uint8_t parentUUID[16];       // rfc 4122 encoded
        uint16_t parentJointIndex;
    } PACKED_END;
    const size_t PARENT_INFO_SIZE = 18;
    static_assert(sizeof(ParentInfo) == PARENT_INFO_SIZE, "AvatarDataPacket::ParentInfo size doesn't match.");

    // will only ever be included if the avatar has a parent but can change independent of changes to parent info
    // and so we keep it a separate record
    PACKED_BEGIN struct AvatarLocalPosition {
        float localPosition[3];           // parent frame translation of the avatar
    } PACKED_END;
    const size_t AVATAR_LOCAL_POSITION_SIZE = 12;
    static_assert(sizeof(AvatarLocalPosition) == AVATAR_LOCAL_POSITION_SIZE, "AvatarDataPacket::AvatarLocalPosition size doesn't match.");

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
        AVATAR_LOCAL_POSITION_SIZE;


    // variable length structure follows

    // only present if IS_FACE_TRACKER_CONNECTED flag is set in AvatarInfo.flags
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
        SixByteTrans translation[numValidTranslations];        // encodeded and compressed by packFloatVec3ToSignedTwoByteFixed()

        SixByteQuat leftHandControllerRotation;
        SixByteTrans leftHandControllerTranslation;
        SixByteQuat rightHandControllerRotation;
        SixByteTrans rightHandControllerTranslation;
    };
    */
    size_t maxJointDataSize(size_t numJoints, bool hasGrabJoints);

    /*
    struct JointDefaultPoseFlags {
       uint8_t numJoints;
       uint8_t rotationIsDefaultPoseBits[ceil(numJoints / 8)];
       uint8_t translationIsDefaultPoseBits[ceil(numJoints / 8)];
    };
    */
    size_t maxJointDefaultPoseFlagsSize(size_t numJoints);

    PACKED_BEGIN struct FarGrabJoints {
        float leftFarGrabPosition[3]; // left controller far-grab joint position
        float leftFarGrabRotation[4]; // left controller far-grab joint rotation
        float rightFarGrabPosition[3]; // right controller far-grab joint position
        float rightFarGrabRotation[4]; // right controller far-grab joint rotation
        float mouseFarGrabPosition[3]; // mouse far-grab joint position
        float mouseFarGrabRotation[4]; // mouse far-grab joint rotation
    } PACKED_END;
    const size_t FAR_GRAB_JOINTS_SIZE = 84;
    static_assert(sizeof(FarGrabJoints) == FAR_GRAB_JOINTS_SIZE, "AvatarDataPacket::FarGrabJoints size doesn't match.");
}

const float MAX_AUDIO_LOUDNESS = 1000.0f; // close enough for mouth animation

// See also static AvatarData::defaultFullAvatarModelUrl().
const QString DEFAULT_FULL_AVATAR_MODEL_NAME = QString("Default");

// how often should we send a full report about joint rotations, even if they haven't changed?
const float AVATAR_SEND_FULL_UPDATE_RATIO = 0.02f;
// this controls how large a change in joint-rotation must be before the interface sends it to the avatar mixer
const float AVATAR_MIN_ROTATION_DOT = 0.9999999f;
const float AVATAR_MIN_TRANSLATION = 0.0001f;

const float ROTATION_CHANGE_15D = 0.9914449f;
const float ROTATION_CHANGE_45D = 0.9238795f;
const float ROTATION_CHANGE_90D = 0.7071068f;
const float ROTATION_CHANGE_179D = 0.0087266f;

const float AVATAR_DISTANCE_LEVEL_1 = 10.0f;
const float AVATAR_DISTANCE_LEVEL_2 = 100.0f;
const float AVATAR_DISTANCE_LEVEL_3 = 1000.0f;
const float AVATAR_DISTANCE_LEVEL_4 = 10000.0f;


// Where one's own Avatar begins in the world (will be overwritten if avatar data file is found).
// This is the start location in the Sandbox (xyz: 6270, 211, 6000).
const glm::vec3 START_LOCATION(6270, 211, 6000);

enum KeyState {
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

class AttachmentData;
class Transform;
using TransformPointer = std::shared_ptr<Transform>;

class AvatarDataRate {
public:
    RateCounter<> globalPositionRate;
    RateCounter<> localPositionRate;
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

class AvatarPriority {
public:
    AvatarPriority(AvatarSharedPointer a, float p) : avatar(a), priority(p) {}
    AvatarSharedPointer avatar;
    float priority;
    // NOTE: we invert the less-than operator to sort high priorities to front
    bool operator<(const AvatarPriority& other) const { return priority < other.priority; }
};

class ClientTraitsHandler;

class AvatarData : public QObject, public SpatiallyNestable {
    Q_OBJECT

    // The following properties have JSDoc in MyAvatar.h and ScriptableAvatar.h
    Q_PROPERTY(glm::vec3 position READ getWorldPosition WRITE setPositionViaScript)
    Q_PROPERTY(float scale READ getDomainLimitedScale WRITE setTargetScale)
    Q_PROPERTY(float density READ getDensity)
    Q_PROPERTY(glm::vec3 handPosition READ getHandPosition WRITE setHandPosition)
    Q_PROPERTY(float bodyYaw READ getBodyYaw WRITE setBodyYaw)
    Q_PROPERTY(float bodyPitch READ getBodyPitch WRITE setBodyPitch)
    Q_PROPERTY(float bodyRoll READ getBodyRoll WRITE setBodyRoll)

    Q_PROPERTY(glm::quat orientation READ getWorldOrientation WRITE setOrientationViaScript)
    Q_PROPERTY(glm::quat headOrientation READ getHeadOrientation WRITE setHeadOrientation)
    Q_PROPERTY(float headPitch READ getHeadPitch WRITE setHeadPitch)
    Q_PROPERTY(float headYaw READ getHeadYaw WRITE setHeadYaw)
    Q_PROPERTY(float headRoll READ getHeadRoll WRITE setHeadRoll)

    Q_PROPERTY(glm::vec3 velocity READ getWorldVelocity WRITE setWorldVelocity)
    Q_PROPERTY(glm::vec3 angularVelocity READ getWorldAngularVelocity WRITE setWorldAngularVelocity)

    Q_PROPERTY(float audioLoudness READ getAudioLoudness WRITE setAudioLoudness)
    Q_PROPERTY(float audioAverageLoudness READ getAudioAverageLoudness WRITE setAudioAverageLoudness)

    Q_PROPERTY(QString displayName READ getDisplayName WRITE setDisplayName NOTIFY displayNameChanged)
    // sessionDisplayName is sanitized, defaulted version displayName that is defined by the AvatarMixer rather than by Interface clients.
    // The result is unique among all avatars present at the time.
    Q_PROPERTY(QString sessionDisplayName READ getSessionDisplayName WRITE setSessionDisplayName NOTIFY sessionDisplayNameChanged)
    Q_PROPERTY(bool lookAtSnappingEnabled MEMBER _lookAtSnappingEnabled NOTIFY lookAtSnappingChanged)
    Q_PROPERTY(QString skeletonModelURL READ getSkeletonModelURLFromScript WRITE setSkeletonModelURLFromScript NOTIFY skeletonModelURLChanged)
    Q_PROPERTY(QVector<AttachmentData> attachmentData READ getAttachmentData WRITE setAttachmentData)

    Q_PROPERTY(QStringList jointNames READ getJointNames)

    Q_PROPERTY(QUuid sessionUUID READ getSessionUUID NOTIFY sessionUUIDChanged)

    Q_PROPERTY(glm::mat4 sensorToWorldMatrix READ getSensorToWorldMatrix)
    Q_PROPERTY(glm::mat4 controllerLeftHandMatrix READ getControllerLeftHandMatrix)
    Q_PROPERTY(glm::mat4 controllerRightHandMatrix READ getControllerRightHandMatrix)

    Q_PROPERTY(float sensorToWorldScale READ getSensorToWorldScale)

public:
    virtual QString getName() const override { return QString("Avatar:") + _displayName; }

    static const QString FRAME_NAME;

    static void fromFrame(const QByteArray& frameData, AvatarData& avatar, bool useFrameSkeleton = true);
    static QByteArray toFrame(const AvatarData& avatar);

    AvatarData();
    virtual ~AvatarData();

    static const QUrl& defaultFullAvatarModelUrl();

    virtual bool isMyAvatar() const { return false; }

    const QUuid getSessionUUID() const { return getID(); }

    glm::vec3 getHandPosition() const;
    void setHandPosition(const glm::vec3& handPosition);

    typedef enum {
        NoData,
        PALMinimum,
        MinimumData,
        CullSmallData,
        IncludeSmallData,
        SendAllData
    } AvatarDataDetail;

    virtual QByteArray toByteArrayStateful(AvatarDataDetail dataDetail, bool dropFaceTracking = false);

    virtual QByteArray toByteArray(AvatarDataDetail dataDetail, quint64 lastSentTime, const QVector<JointData>& lastSentJointData,
        AvatarDataPacket::HasFlags& hasFlagsOut, bool dropFaceTracking, bool distanceAdjust, glm::vec3 viewerPosition,
        QVector<JointData>* sentJointDataOut, AvatarDataRate* outboundDataRateOut = nullptr) const;

    virtual void doneEncoding(bool cullSmallChanges);

    /// \return true if an error should be logged
    bool shouldLogError(const quint64& now);

    /// \param packet byte array of data
    /// \param offset number of bytes into packet where data starts
    /// \return number of bytes parsed
    virtual int parseDataFromBuffer(const QByteArray& buffer);

    // Body Rotation (degrees)
    float getBodyYaw() const;
    void setBodyYaw(float bodyYaw);
    float getBodyPitch() const;
    void setBodyPitch(float bodyPitch);
    float getBodyRoll() const;
    void setBodyRoll(float bodyRoll);

    virtual void setPositionViaScript(const glm::vec3& position);
    virtual void setOrientationViaScript(const glm::quat& orientation);

    virtual void updateAttitude(const glm::quat& orientation) {}

    glm::quat getHeadOrientation() const {
        lazyInitHeadData();
        return _headData->getOrientation();
    }
    void setHeadOrientation(const glm::quat& orientation) {
        if (_headData) {
            _headData->setOrientation(orientation);
        }
    }

    void setLookAtPosition(const glm::vec3& lookAtPosition) {
        if (_headData) {
            _headData->setLookAtPosition(lookAtPosition);
        }
    }

    void setBlendshapeCoefficients(const QVector<float>& blendshapeCoefficients) {
        if (_headData) {
            _headData->setBlendshapeCoefficients(blendshapeCoefficients);
        }
    }

    // access to Head().set/getMousePitch (degrees)
    float getHeadPitch() const { return _headData->getBasePitch(); }
    void setHeadPitch(float value) { _headData->setBasePitch(value); }

    float getHeadYaw() const { return _headData->getBaseYaw(); }
    void setHeadYaw(float value) { _headData->setBaseYaw(value); }

    float getHeadRoll() const { return _headData->getBaseRoll(); }
    void setHeadRoll(float value) { _headData->setBaseRoll(value); }

    // access to Head().set/getAverageLoudness

    float getAudioLoudness() const { return _audioLoudness; }
    void setAudioLoudness(float audioLoudness) {
        if (audioLoudness != _audioLoudness) {
            _audioLoudnessChanged = usecTimestampNow();
        }
        _audioLoudness = audioLoudness;
    }
    bool audioLoudnessChangedSince(quint64 time) const { return _audioLoudnessChanged >= time; }

    float getAudioAverageLoudness() const { return _audioAverageLoudness; }
    void setAudioAverageLoudness(float audioAverageLoudness) { _audioAverageLoudness = audioAverageLoudness; }

    //  Scale
    virtual void setTargetScale(float targetScale);

    float getDomainLimitedScale() const;

    virtual bool getHasScriptedBlendshapes() const { return false; }
    virtual bool getHasProceduralBlinkFaceMovement() const { return true; }
    virtual bool getHasProceduralEyeFaceMovement() const { return true; }
    virtual bool getHasAudioEnabledFaceMovement() const { return false; }

    /**jsdoc
     * Returns the minimum scale allowed for this avatar in the current domain.
     * This value can change as the user changes avatars or when changing domains.
     * @function MyAvatar.getDomainMinScale
     * @returns {number} minimum scale allowed for this avatar in the current domain.
     */
    Q_INVOKABLE float getDomainMinScale() const;

    /**jsdoc
     * Returns the maximum scale allowed for this avatar in the current domain.
     * This value can change as the user changes avatars or when changing domains.
     * @function MyAvatar.getDomainMaxScale
     * @returns {number} maximum scale allowed for this avatar in the current domain.
     */
    Q_INVOKABLE float getDomainMaxScale() const;

    // Returns eye height of avatar in meters, ignoring avatar scale.
    // if _targetScale is 1 then this will be identical to getEyeHeight;
    virtual float getUnscaledEyeHeight() const { return DEFAULT_AVATAR_EYE_HEIGHT; }

    // returns true, if an acurate eye height estimage can be obtained by inspecting the avatar model skeleton and geometry,
    // not all subclasses of AvatarData have access to this data.
    virtual bool canMeasureEyeHeight() const { return false; }

    /**jsdoc
     * Provides read only access to the current eye height of the avatar.
     * This height is only an estimate and might be incorrect for avatars that are missing standard joints.
     * @function MyAvatar.getEyeHeight
     * @returns {number} Eye height of avatar in meters.
     */
    Q_INVOKABLE virtual float getEyeHeight() const { return _targetScale * getUnscaledEyeHeight(); }

    /**jsdoc
     * Provides read only access to the current height of the avatar.
     * This height is only an estimate and might be incorrect for avatars that are missing standard joints.
     * @function MyAvatar.getHeight
     * @returns {number} Height of avatar in meters.
     */
    Q_INVOKABLE virtual float getHeight() const;

    float getUnscaledHeight() const;

    void setDomainMinimumHeight(float domainMinimumHeight);
    void setDomainMaximumHeight(float domainMaximumHeight);

    /**jsdoc
     * @function MyAvatar.setHandState
     * @param {string} state
     */
    Q_INVOKABLE void setHandState(char s) { _handState = s; }

    /**jsdoc
     * @function MyAvatar.getHandState
     * @returns {string} 
     */
    Q_INVOKABLE char getHandState() const { return _handState; }

    const QVector<JointData>& getRawJointData() const { return _jointData; }

    /**jsdoc
     * @function MyAvatar.setRawJointData
     * @param {JointData[]} data
     */
    Q_INVOKABLE void setRawJointData(QVector<JointData> data);

    /**jsdoc
     * Set a specific joint's rotation and position relative to its parent.
     * <p>Setting joint data completely overrides/replaces all motion from the default animation system including inverse 
     * kinematics, but just for the specified joint. So for example, if you were to procedurally manipulate the finger joints, 
     * the avatar's hand and head would still do inverse kinematics properly. However, as soon as you start to manipulate 
     * joints in the inverse kinematics chain, the inverse kinematics might not function as you expect. For example, if you set 
     * the rotation of the elbow, the hand inverse kinematics position won't end up in the right place.</p>
     * @function MyAvatar.setJointData
     * @param {number} index - The index of the joint.
     * @param {Quat} rotation - The rotation of the joint relative to its parent.
     * @param {Vec3} translation - The translation of the joint relative to its parent.
     * @example <caption>Set your avatar to it's default T-pose for a while.<br />
     * <img alt="Avatar in T-pose" src="https://docs.highfidelity.com/images/t-pose.png" /></caption>
     * // Set all joint translations and rotations to defaults.
     * var i, length, rotation, translation;
     * for (i = 0, length = MyAvatar.getJointNames().length; i < length; i++) {
     *     rotation = MyAvatar.getDefaultJointRotation(i);
     *     translation = MyAvatar.getDefaultJointTranslation(i);
     *     MyAvatar.setJointData(i, rotation, translation);
     * }
     *
     * // Restore your avatar's motion after 5s.
     * Script.setTimeout(function () {
     *     MyAvatar.clearJointsData();
     * }, 5000);
     */
    Q_INVOKABLE virtual void setJointData(int index, const glm::quat& rotation, const glm::vec3& translation);

    /**jsdoc
     * Set a specific joint's rotation relative to its parent.
     * <p>Setting joint data completely overrides/replaces all motion from the default animation system including inverse 
     * kinematics, but just for the specified joint. So for example, if you were to procedurally manipulate the finger joints, 
     * the avatar's hand and head would still do inverse kinematics properly. However, as soon as you start to manipulate 
     * joints in the inverse kinematics chain, the inverse kinematics might not function as you expect. For example, if you set 
     * the rotation of the elbow, the hand inverse kinematics position won't end up in the right place.</p>
     * @function MyAvatar.setJointRotation
     * @param {number} index - The index of the joint.
     * @param {Quat} rotation - The rotation of the joint relative to its parent.
     */
    Q_INVOKABLE virtual void setJointRotation(int index, const glm::quat& rotation);

    /**jsdoc
     * Set a specific joint's translation relative to its parent.
     * <p>Setting joint data completely overrides/replaces all motion from the default animation system including inverse 
     * kinematics, but just for the specified joint. So for example, if you were to procedurally manipulate the finger joints, 
     * the avatar's hand and head would still do inverse kinematics properly. However, as soon as you start to manipulate 
     * joints in the inverse kinematics chain, the inverse kinematics might not function as you expect. For example, if you set 
     * the rotation of the elbow, the hand inverse kinematics position won't end up in the right place.</p>
     * @function MyAvatar.setJointTranslation
     * @param {number} index - The index of the joint.
     * @param {Vec3} translation - The translation of the joint relative to its parent.
     */
    Q_INVOKABLE virtual void setJointTranslation(int index, const glm::vec3& translation);

    /**jsdoc
     * Clear joint translations and rotations set by script for a specific joint. This restores all motion from the default 
     * animation system including inverse kinematics for that joint.
     * <p>Note: This is slightly faster than the function variation that specifies the joint name.</p>
     * @function MyAvatar.clearJointData
     * @param {number} index - The index of the joint.
     */
    Q_INVOKABLE virtual void clearJointData(int index);

    /**jsdoc
     * @function MyAvatar.isJointDataValid
     * @param {number} index
     * @returns {boolean} 
     */
    Q_INVOKABLE bool isJointDataValid(int index) const;

    /**jsdoc
     * Get the rotation of a joint relative to its parent. For information on the joint hierarchy used, see 
     * <a href="https://docs.highfidelity.com/create-and-explore/avatars/avatar-standards">Avatar Standards</a>.
     * @function MyAvatar.getJointRotation
     * @param {number} index - The index of the joint.
     * @returns {Quat} The rotation of the joint relative to its parent.
     */
    Q_INVOKABLE virtual glm::quat getJointRotation(int index) const;

    /**jsdoc
     * Get the translation of a joint relative to its parent. For information on the joint hierarchy used, see 
     * <a href="https://docs.highfidelity.com/create-and-explore/avatars/avatar-standards">Avatar Standards</a>.
     * @function MyAvatar.getJointTranslation
     * @param {number} index - The index of the joint.
     * @returns {Vec3} The translation of the joint relative to its parent.
     */
    Q_INVOKABLE virtual glm::vec3 getJointTranslation(int index) const;

    /**jsdoc
     * Set a specific joint's rotation and position relative to its parent.
     * <p>Setting joint data completely overrides/replaces all motion from the default animation system including inverse 
     * kinematics, but just for the specified joint. So for example, if you were to procedurally manipulate the finger joints, 
     * the avatar's hand and head would still do inverse kinematics properly. However, as soon as you start to manipulate 
     * joints in the inverse kinematics chain, the inverse kinematics might not function as you expect. For example, if you set 
     * the rotation of the elbow, the hand inverse kinematics position won't end up in the right place.</p>
     * @function MyAvatar.setJointData
     * @param {string} name - The name of the joint.
     * @param {Quat} rotation - The rotation of the joint relative to its parent.
     * @param {Vec3} translation - The translation of the joint relative to its parent.
     */
    Q_INVOKABLE virtual void setJointData(const QString& name, const glm::quat& rotation, const glm::vec3& translation);

    /**jsdoc
     * Set a specific joint's rotation relative to its parent.
     * <p>Setting joint data completely overrides/replaces all motion from the default animation system including inverse 
     * kinematics, but just for the specified joint. So for example, if you were to procedurally manipulate the finger joints, 
     * the avatar's hand and head would still do inverse kinematics properly. However, as soon as you start to manipulate 
     * joints in the inverse kinematics chain, the inverse kinematics might not function as you expect. For example, if you set 
     * the rotation of the elbow, the hand inverse kinematics position won't end up in the right place.</p>
     * @function MyAvatar.setJointRotation
     * @param {string} name - The name of the joint.
     * @param {Quat} rotation - The rotation of the joint relative to its parent.
     * @example <caption>Set your avatar to its default T-pose then rotate its right arm.<br />
     * <img alt="Avatar in T-pose with arm rotated" src="https://docs.highfidelity.com/images/armpose.png" /></caption>
     * // Set all joint translations and rotations to defaults.
     * var i, length, rotation, translation;
     * for (i = 0, length = MyAvatar.getJointNames().length; i < length; i++) {
     *     rotation = MyAvatar.getDefaultJointRotation(i);
     *     translation = MyAvatar.getDefaultJointTranslation(i);
     *     MyAvatar.setJointData(i, rotation, translation);
     * }
     *
     * // Rotate the right arm.
     * var newArmRotation = { x: 0.47, y: 0.22, z: -0.02, w: 0.87 };
     * MyAvatar.setJointRotation("RightArm", newArmRotation);
     *
     * // Restore your avatar's motion after 5s.
     * Script.setTimeout(function () {
     *     MyAvatar.clearJointsData();
     * }, 5000);
     */
    Q_INVOKABLE virtual void setJointRotation(const QString& name, const glm::quat& rotation);

    /**jsdoc
     * Set a specific joint's translation relative to its parent.
     * <p>Setting joint data completely overrides/replaces all motion from the default animation system including inverse 
     * kinematics, but just for the specified joint. So for example, if you were to procedurally manipulate the finger joints, 
     * the avatar's hand and head would still do inverse kinematics properly. However, as soon as you start to manipulate 
     * joints in the inverse kinematics chain, the inverse kinematics might not function as you expect. For example, if you set 
     * the rotation of the elbow, the hand inverse kinematics position won't end up in the right place.</p>
     * @function MyAvatar.setJointTranslation
     * @param {string} name - The name of the joint.
     * @param {Vec3} translation - The translation of the joint relative to its parent.
     * @example <caption>Stretch your avatar's neck. Depending on the avatar you are using, you will either see a gap between 
     * the head and body or you will see the neck stretched.<br />
     * <img alt="Avatar with neck stretched" src="https://docs.highfidelity.com/images/stretched-neck.png" /></caption>
     * // Stretch your avatar's neck.
     * MyAvatar.setJointTranslation("Neck", { x: 0, y: 25, z: 0 });
     * 
     * // Restore your avatar's neck after 5s.
     * Script.setTimeout(function () {
     *     MyAvatar.clearJointData("Neck");
     * }, 5000);
     */
    Q_INVOKABLE virtual void setJointTranslation(const QString& name, const glm::vec3& translation);

    /**jsdoc
     * Clear joint translations and rotations set by script for a specific joint. This restores all motion from the default 
     * animation system including inverse kinematics for that joint.
     * <p>Note: This is slightly slower than the function variation that specifies the joint index.</p>
     * @function MyAvatar.clearJointData
     * @param {string} name - The name of the joint.
     * @example <caption>Offset and restore the position of your avatar's head.</caption>
     * // Move your avatar's head up by 25cm from where it should be.
     * MyAvatar.setJointTranslation("Neck", { x: 0, y: 0.25, z: 0 });
     *
     * // Restore your avatar's head to its default position after 5s.
     * Script.setTimeout(function () {
     *     MyAvatar.clearJointData("Neck");
     * }, 5000);
     */
    Q_INVOKABLE virtual void clearJointData(const QString& name);

    /**jsdoc
     * @function MyAvatar.isJointDataValid
     * @param {string} name
     * @returns {boolean} 
     */
    Q_INVOKABLE virtual bool isJointDataValid(const QString& name) const;

    /**jsdoc
     * Get the rotation of a joint relative to its parent. For information on the joint hierarchy used, see 
     * <a href="https://docs.highfidelity.com/create-and-explore/avatars/avatar-standards">Avatar Standards</a>.
     * @function MyAvatar.getJointRotation
     * @param {string} name - The name of the joint.
     * @returns {Quat} The rotation of the joint relative to its parent.
     * @example <caption>Report the rotation of your avatar's hips joint.</caption>
     * print(JSON.stringify(MyAvatar.getJointRotation("Hips")));
     */
    Q_INVOKABLE virtual glm::quat getJointRotation(const QString& name) const;

    /**jsdoc
     * Get the translation of a joint relative to its parent. For information on the joint hierarchy used, see 
     * <a href="https://docs.highfidelity.com/create-and-explore/avatars/avatar-standards">Avatar Standards</a>.
     * @function MyAvatar.getJointTranslation
     * @param {number} name - The name of the joint.
     * @returns {Vec3} The translation of the joint relative to its parent.
     * @example <caption>Report the translation of your avatar's hips joint.</caption>
     * print(JSON.stringify(MyAvatar.getJointRotation("Hips")));
     */
    Q_INVOKABLE virtual glm::vec3 getJointTranslation(const QString& name) const;

    /**jsdoc
     * Get the rotations of all joints in the current avatar. Each joint's rotation is relative to its parent joint.
     * @function MyAvatar.getJointRotations
     * @returns {Quat[]} The rotations of all joints relative to each's parent. The values are in the same order as the array 
     * returned by {@link MyAvatar.getJointNames} or {@link Avatar.getJointNames}.
     * @example <caption>Report the rotations of all your avatar's joints.</caption>
     * print(JSON.stringify(MyAvatar.getJointRotations()));
     */
    Q_INVOKABLE virtual QVector<glm::quat> getJointRotations() const;

    /**jsdoc
     * @function MyAvatar.getJointTranslations
     * @returns {Vec3[]} 
     */
    Q_INVOKABLE virtual QVector<glm::vec3> getJointTranslations() const;

    /**jsdoc
     * Set the rotations of all joints in the current avatar. Each joint's rotation is relative to its parent joint.
     * <p>Setting joint data completely overrides/replaces all motion from the default animation system including inverse
     * kinematics, but just for the specified joint. So for example, if you were to procedurally manipulate the finger joints,
     * the avatar's hand and head would still do inverse kinematics properly. However, as soon as you start to manipulate
     * joints in the inverse kinematics chain, the inverse kinematics might not function as you expect. For example, if you set
     * the rotation of the elbow, the hand inverse kinematics position won't end up in the right place.</p>
     * @function MyAvatar.setJointRotations
     * @param {Quat[]} jointRotations - The rotations for all joints in the avatar. The values are in the same order as the 
     * array returned by {@link MyAvatar.getJointNames} or {@link Avatar.getJointNames}.
     * @example <caption>Set your avatar to its default T-pose then rotate its right arm.<br />
     * <img alt="Avatar in T-pose" src="https://docs.highfidelity.com/images/armpose.png" /></caption>
     * // Set all joint translations and rotations to defaults.
     * var i, length, rotation, translation;
     * for (i = 0, length = MyAvatar.getJointNames().length; i < length; i++) {
     *     rotation = MyAvatar.getDefaultJointRotation(i);
     *     translation = MyAvatar.getDefaultJointTranslation(i);
     *     MyAvatar.setJointData(i, rotation, translation);
     * }
     *
     * // Get all join rotations.
     * var jointRotations = MyAvatar.getJointRotations(); 
     *
     * // Update the rotation of the right arm in the array.
     * jointRotations[MyAvatar.getJointIndex("RightArm")] = { x: 0.47, y: 0.22, z: -0.02, w: 0.87 };
     *
     * // Update all joint rotations.
     * MyAvatar.setJointRotations(jointRotations);
     *
     * // Restore your avatar's motion after 5s.
     * Script.setTimeout(function () {
     *     MyAvatar.clearJointsData();
     * }, 5000);
     */
    Q_INVOKABLE virtual void setJointRotations(const QVector<glm::quat>& jointRotations);
    
    /**jsdoc
     * @function MyAvatar.setJointTranslations
     * @param {Vec3[]} translations
     */
    Q_INVOKABLE virtual void setJointTranslations(const QVector<glm::vec3>& jointTranslations);

    /**jsdoc
     * Clear all joint translations and rotations that have been set by script. This restores all motion from the default 
     * animation system including inverse kinematics for all joints.
     * @function MyAvatar.clearJointsData
     * @example <caption>Set your avatar to it's default T-pose for a while.</caption>
     * // Set all joint translations and rotations to defaults.
     * var i, length, rotation, translation;
     * for (i = 0, length = MyAvatar.getJointNames().length; i < length; i++) {
     *     rotation = MyAvatar.getDefaultJointRotation(i);
     *     translation = MyAvatar.getDefaultJointTranslation(i);
     *     MyAvatar.setJointData(i, rotation, translation);
     * }
     *
     * // Restore your avatar's motion after 5s.
     * Script.setTimeout(function () {
     *     MyAvatar.clearJointsData();
     * }, 5000);
     */
    Q_INVOKABLE virtual void clearJointsData();

    /**jsdoc
     * Get the joint index for a named joint. The joint index value is the position of the joint in the array returned by 
     * {@link MyAvatar.getJointNames} or {@link Avatar.getJointNames}.
     * @function MyAvatar.getJointIndex
     * @param {string} name - The name of the joint.
     * @returns {number} The index of the joint.
     * @example <caption>Report the index of your avatar's left arm joint.</caption>
     * print(JSON.stringify(MyAvatar.getJointIndex("LeftArm"));
     */
    /// Returns the index of the joint with the specified name, or -1 if not found/unknown.
    Q_INVOKABLE virtual int getJointIndex(const QString& name) const;

    /**jsdoc
     * Get the names of all the joints in the current avatar.
     * @function MyAvatar.getJointNames
     * @returns {string[]} The joint names.
     * @example <caption>Report the names of all the joints in your current avatar.</caption>
     * print(JSON.stringify(MyAvatar.getJointNames()));
     */
    Q_INVOKABLE virtual QStringList getJointNames() const;


    /**jsdoc
     * @function MyAvatar.setBlendshape
     * @param {string} name
     * @param {number} value
     */
    Q_INVOKABLE void setBlendshape(QString name, float val) { _headData->setBlendshape(name, val); }


    /**jsdoc
     * @function MyAvatar.getAttachmentsVariant
     * @returns {object} 
     */
    // FIXME: Can this name be improved? Can it be deprecated?
    Q_INVOKABLE virtual QVariantList getAttachmentsVariant() const;

    /**jsdoc
     * @function MyAvatar.setAttachmentsVariant
     * @param {object} variant
     */
    // FIXME: Can this name be improved? Can it be deprecated?
    Q_INVOKABLE virtual void setAttachmentsVariant(const QVariantList& variant);


    /**jsdoc
     * @function MyAvatar.updateAvatarEntity
     * @param {Uuid} entityID
     * @param {string} entityData
     */
    Q_INVOKABLE void updateAvatarEntity(const QUuid& entityID, const QByteArray& entityData);

    /**jsdoc
     * @function MyAvatar.clearAvatarEntity
     * @param {Uuid} entityID
     */
    Q_INVOKABLE void clearAvatarEntity(const QUuid& entityID, bool requiresRemovalFromTree = true);


    /**jsdoc
     * @function MyAvatar.setForceFaceTrackerConnected
     * @param {boolean} connected
     */
    Q_INVOKABLE void setForceFaceTrackerConnected(bool connected) { _forceFaceTrackerConnected = connected; }

    // key state
    void setKeyState(KeyState s) { _keyState = s; }
    KeyState keyState() const { return _keyState; }

    const HeadData* getHeadData() const { return _headData; }

    struct Identity {
        QVector<AttachmentData> attachmentData;
        QString displayName;
        QString sessionDisplayName;
        bool isReplicated;
        bool lookAtSnappingEnabled;
    };

    // identityChanged returns true if identity has changed, false otherwise.
    // identityChanged returns true if identity has changed, false otherwise. Similarly for displayNameChanged and skeletonModelUrlChange.
    void processAvatarIdentity(const QByteArray& identityData, bool& identityChanged, bool& displayNameChanged);

    qint64 packTrait(AvatarTraits::TraitType traitType, ExtendedIODevice& destination,
                     AvatarTraits::TraitVersion traitVersion = AvatarTraits::NULL_TRAIT_VERSION);
    qint64 packTraitInstance(AvatarTraits::TraitType traitType, AvatarTraits::TraitInstanceID instanceID,
                             ExtendedIODevice& destination, AvatarTraits::TraitVersion traitVersion = AvatarTraits::NULL_TRAIT_VERSION);

    void prepareResetTraitInstances();

    void processTrait(AvatarTraits::TraitType traitType, QByteArray traitBinaryData);
    void processTraitInstance(AvatarTraits::TraitType traitType,
                              AvatarTraits::TraitInstanceID instanceID, QByteArray traitBinaryData);
    void processDeletedTraitInstance(AvatarTraits::TraitType traitType, AvatarTraits::TraitInstanceID instanceID);

    QByteArray identityByteArray(bool setIsReplicated = false) const;

    QUrl getWireSafeSkeletonModelURL() const;
    const QUrl& getSkeletonModelURL() const { return _skeletonModelURL; }

    const QString& getDisplayName() const { return _displayName; }
    const QString& getSessionDisplayName() const { return _sessionDisplayName; }
    bool getLookAtSnappingEnabled() const { return _lookAtSnappingEnabled; }
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);

    virtual void setDisplayName(const QString& displayName);
    virtual void setSessionDisplayName(const QString& sessionDisplayName) {
        _sessionDisplayName = sessionDisplayName;
        markIdentityDataChanged();
    }

    /**jsdoc
     * Get information about all models currently attached to your avatar.
     * @function MyAvatar.getAttachmentData
     * @returns {AttachmentData[]} Information about all models attached to your avatar.
     * @example <caption>Report the URLs of all current attachments.</caption>
     * var attachments = MyAvatar.getaAttachmentData();
     * for (var i = 0; i < attachments.length; i++) {
     *     print (attachments[i].modelURL);
     * }
     */
    Q_INVOKABLE virtual QVector<AttachmentData> getAttachmentData() const;

    /**jsdoc
     * Set all models currently attached to your avatar. For example, if you retrieve attachment data using 
     * {@link MyAvatar.getAttachmentData} or {@link Avatar.getAttachmentData}, make changes to it, and then want to update your avatar's attachments per the 
     * changed data. You can also remove all attachments by using setting <code>attachmentData</code> to <code>null</code>.
     * @function MyAvatar.setAttachmentData
     * @param {AttachmentData[]} attachmentData - The attachment data defining the models to have attached to your avatar. Use 
     *     <code>null</code> to remove all attachments.
     * @example <caption>Remove a hat attachment if your avatar is wearing it.</caption>
     * var hatURL = "https://s3.amazonaws.com/hifi-public/tony/cowboy-hat.fbx";
     * var attachments = MyAvatar.getAttachmentData();
     *
     * for (var i = 0; i < attachments.length; i++) {
     *     if (attachments[i].modelURL === hatURL) {
     *         attachments.splice(i, 1);
     *         MyAvatar.setAttachmentData(attachments);
     *         break;
     *     }
     *  }
     */
    Q_INVOKABLE virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData);

    /**jsdoc
     * Attach a model to your avatar. For example, you can give your avatar a hat to wear, a guitar to hold, or a surfboard to 
     * stand on.
     * <p>Note: Attached models are models only; they are not entities and can not be manipulated using the {@link Entities} API. 
     * Nor can you use this function to attach an entity (such as a sphere or a box) to your avatar.</p>
     * @function MyAvatar.attach
     * @param {string} modelURL - The URL of the model to attach. Models can be .FBX or .OBJ format.
     * @param {string} [jointName=""] - The name of the avatar joint (see {@link MyAvatar.getJointNames} or {@link Avatar.getJointNames}) to attach the model 
     *     to.
     * @param {Vec3} [translation=Vec3.ZERO] - The offset to apply to the model relative to the joint position.
     * @param {Quat} [rotation=Quat.IDENTITY] - The rotation to apply to the model relative to the joint orientation.
     * @param {number} [scale=1.0] - The scale to apply to the model.
     * @param {boolean} [isSoft=false] -  If the model has a skeleton, set this to <code>true</code> so that the bones of the 
     *     attached model's skeleton are be rotated to fit the avatar's current pose. <code>isSoft</code> is used, for example, 
     *     to have clothing that moves with the avatar.<br />
     *     If <code>true</code>, the <code>translation</code>, <code>rotation</code>, and <code>scale</code> parameters are 
     *     ignored.
     * @param {boolean} [allowDuplicates=false]
     * @param {boolean} [useSaved=true]
     * @example <caption>Attach a cowboy hat to your avatar's head.</caption>
     * var attachment = {
     *     modelURL: "https://s3.amazonaws.com/hifi-public/tony/cowboy-hat.fbx",
     *     jointName: "Head",
     *     translation: {"x": 0, "y": 0.25, "z": 0},
     *     rotation: {"x": 0, "y": 0, "z": 0, "w": 1},
     *     scale: 1,
     *     isSoft: false
     * };
     *
     *  MyAvatar.attach(attachment.modelURL,
     *                  attachment.jointName,
     *                  attachment.translation,
     *                  attachment.rotation,
     *                  attachment.scale,
     *                  attachment.isSoft);
     */
    Q_INVOKABLE virtual void attach(const QString& modelURL, const QString& jointName = QString(),
                                    const glm::vec3& translation = glm::vec3(), const glm::quat& rotation = glm::quat(),
                                    float scale = 1.0f, bool isSoft = false,
                                    bool allowDuplicates = false, bool useSaved = true);

    /**jsdoc
     * Detach the most recently attached instance of a particular model from either a specific joint or any joint.
     * @function MyAvatar.detachOne
     * @param {string} modelURL - The URL of the model to detach.
     * @param {string} [jointName=""] - The name of the joint to detach the model from. If <code>""</code>, then the most 
     *     recently attached model is removed from which ever joint it was attached to.
     */
    Q_INVOKABLE virtual void detachOne(const QString& modelURL, const QString& jointName = QString());

    /**jsdoc
     * Detach all instances of a particular model from either a specific joint or all joints.
     * @function MyAvatar.detachAll
     * @param {string} modelURL - The URL of the model to detach.
     * @param {string} [jointName=""] - The name of the joint to detach the model from. If <code>""</code>, then the model is 
     *     detached from all joints.
     */
    Q_INVOKABLE virtual void detachAll(const QString& modelURL, const QString& jointName = QString());

    QString getSkeletonModelURLFromScript() const { return _skeletonModelURL.toString(); }
    void setSkeletonModelURLFromScript(const QString& skeletonModelString) { setSkeletonModelURL(QUrl(skeletonModelString)); }

    void setOwningAvatarMixer(const QWeakPointer<Node>& owningAvatarMixer) { _owningAvatarMixer = owningAvatarMixer; }

    int getAverageBytesReceivedPerSecond() const;
    int getReceiveRate() const;

    const glm::vec3& getTargetVelocity() const { return _targetVelocity; }

    void clearRecordingBasis();
    TransformPointer getRecordingBasis() const;
    void setRecordingBasis(TransformPointer recordingBasis = TransformPointer());
    void createRecordingIDs();
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json, bool useFrameSkeleton = true);

    glm::vec3 getClientGlobalPosition() const { return _globalPosition; }
    AABox getGlobalBoundingBox() const { return AABox(_globalPosition + _globalBoundingBoxOffset - _globalBoundingBoxDimensions, _globalBoundingBoxDimensions); }

    /**jsdoc
     * @function MyAvatar.getAvatarEntityData
     * @returns {object} 
     */
    Q_INVOKABLE AvatarEntityMap getAvatarEntityData() const;

    /**jsdoc
     * @function MyAvatar.setAvatarEntityData
     * @param {object} avatarEntityData
     */
    Q_INVOKABLE void setAvatarEntityData(const AvatarEntityMap& avatarEntityData);

    virtual void setAvatarEntityDataChanged(bool value) { _avatarEntityDataChanged = value; }
    void insertDetachedEntityID(const QUuid entityID);
    AvatarEntityIDs getAndClearRecentlyDetachedIDs();

    /**jsdoc
     * @function MyAvatar.getSensorToWorldMatrix
     * @returns {Mat4} 
     */
    // thread safe
    Q_INVOKABLE glm::mat4 getSensorToWorldMatrix() const;

    /**jsdoc
     * @function MyAvatar.getSensorToWorldScale
     * @returns {number} 
     */
    // thread safe
    Q_INVOKABLE float getSensorToWorldScale() const;

    /**jsdoc
     * @function MyAvatar.getControllerLeftHandMatrix
     * @returns {Mat4} 
     */
    // thread safe
    Q_INVOKABLE glm::mat4 getControllerLeftHandMatrix() const;

    /**jsdoc
     * @function MyAvatar.getControllerRightHandMatrix
     * @returns {Mat4} 
     */
    // thread safe
    Q_INVOKABLE glm::mat4 getControllerRightHandMatrix() const;


    /**jsdoc
     * @function MyAvatar.getDataRate
     * @param {string} [rateName=""]
     * @returns {number} 
     */
    Q_INVOKABLE float getDataRate(const QString& rateName = QString("")) const;

    /**jsdoc
     * @function MyAvatar.getUpdateRate
     * @param {string} [rateName=""]
     * @returns {number} 
     */
    Q_INVOKABLE float getUpdateRate(const QString& rateName = QString("")) const;

    int getJointCount() const { return _jointData.size(); }

    QVector<JointData> getLastSentJointData() {
        QReadLocker readLock(&_jointDataLock);
        _lastSentJointData.resize(_jointData.size());
        return _lastSentJointData;
    }

    // A method intended to be overriden by MyAvatar for polling orientation for network transmission.
    virtual glm::quat getOrientationOutbound() const;

    // TODO: remove this HACK once we settle on optimal sort coefficients
    // These coefficients exposed for fine tuning the sort priority for transfering new _jointData to the render pipeline.
    static float _avatarSortCoefficientSize;
    static float _avatarSortCoefficientCenter;
    static float _avatarSortCoefficientAge;

    bool getIdentityDataChanged() const { return _identityDataChanged; } // has the identity data changed since the last time sendIdentityPacket() was called
    void markIdentityDataChanged() { _identityDataChanged = true; }

    void pushIdentitySequenceNumber() { ++_identitySequenceNumber; };
    bool hasProcessedFirstIdentity() const { return _hasProcessedFirstIdentity; }

    float getDensity() const { return _density; }

    bool getIsReplicated() const { return _isReplicated; }

    virtual void addMaterial(graphics::MaterialLayer material, const std::string& parentMaterialName) {}
    virtual void removeMaterial(graphics::MaterialPointer material, const std::string& parentMaterialName) {}
    void setReplicaIndex(int replicaIndex) { _replicaIndex = replicaIndex; }
    int getReplicaIndex() { return _replicaIndex; }

signals:

    /**jsdoc
     * @function MyAvatar.displayNameChanged
     * @returns {Signal}
     */
    void displayNameChanged();

    /**jsdoc
     * @function MyAvatar.sessionDisplayNameChanged
     * @returns {Signal}
     */
    void sessionDisplayNameChanged();

    /**jsdoc
     * @function MyAvatar.skeletonModelURLChanged
     * @returns {Signal}
     */
    void skeletonModelURLChanged();

    /**jsdoc
     * @function MyAvatar.lookAtSnappingChanged
     * @param {boolean} enabled
     * @returns {Signal}
     */
    void lookAtSnappingChanged(bool enabled);

    /**jsdoc
     * @function MyAvatar.sessionUUIDChanged
     * @returns {Signal}
     */
    void sessionUUIDChanged();

public slots:

/**jsdoc
     * @function MyAvatar.sendAvatarDataPacket
     * @param {boolean} [sendAll=false]
     */
    void sendAvatarDataPacket(bool sendAll = false);

    /**jsdoc
     * @function MyAvatar.sendIdentityPacket
     */
    void sendIdentityPacket();

    /**jsdoc
     * @function MyAvatar.setJointMappingsFromNetworkReply
     */
    void setJointMappingsFromNetworkReply();

    /**jsdoc
     * @function MyAvatar.setSessionUUID
     * @param {Uuid} sessionUUID
     */
    virtual void setSessionUUID(const QUuid& sessionUUID) {
        if (sessionUUID != getID()) {
            if (sessionUUID == QUuid()) {
                setID(AVATAR_SELF_ID);
            } else {
                setID(sessionUUID);
            }
            emit sessionUUIDChanged();
        }
    }

    /**jsdoc
     * @function MyAvatar.getAbsoluteJointRotationInObjectFrame
     * @param {number} index
     * @returns {Quat} 
     */
    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;

    /**jsdoc
     * @function MyAvatar.getAbsoluteJointTranslationInObjectFrame
     * @param {number} index
     * @returns {Vec3} 
     */
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;

    /**jsdoc
     * @function MyAvatar.setAbsoluteJointRotationInObjectFrame
     * @param {number} index
     * @param {Quat} rotation
     * @returns {boolean}
     */
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override { return false; }

    /**jsdoc
     * @function MyAvatar.setAbsoluteJointTranslationInObjectFrame
     * @param {number} index
     * @param {Vec3} translation
     * @returns {boolean} 
     */
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override { return false; }

    /**jsdoc
     * @function MyAvatar.getTargetScale
     * @returns {number} 
     */
    float getTargetScale() const { return _targetScale; } // why is this a slot?

    /**jsdoc
     * @function MyAvatar.resetLastSent
     */
    void resetLastSent() { _lastToByteArray = 0; }

protected:
    void lazyInitHeadData() const;

    float getDistanceBasedMinRotationDOT(glm::vec3 viewerPosition) const;
    float getDistanceBasedMinTranslationDistance(glm::vec3 viewerPosition) const;

    bool avatarBoundingBoxChangedSince(quint64 time) const { return _avatarBoundingBoxChanged >= time; }
    bool avatarScaleChangedSince(quint64 time) const { return _avatarScaleChanged >= time; }
    bool lookAtPositionChangedSince(quint64 time) const { return _headData->lookAtPositionChangedSince(time); }
    bool sensorToWorldMatrixChangedSince(quint64 time) const { return _sensorToWorldMatrixChanged >= time; }
    bool additionalFlagsChangedSince(quint64 time) const { return _additionalFlagsChanged >= time; }
    bool parentInfoChangedSince(quint64 time) const { return _parentChanged >= time; }
    bool faceTrackerInfoChangedSince(quint64 time) const { return true; } // FIXME

    bool hasParent() const { return !getParentID().isNull(); }
    bool hasFaceTracker() const { return _headData ? _headData->_isFaceTrackerConnected : false; }

    // isReplicated will be true on downstream Avatar Mixers and their clients, but false on the upstream "master"
    // Audio Mixer that the replicated avatar is connected to.
    bool _isReplicated{ false };

    glm::vec3 _handPosition;
    virtual const QString& getSessionDisplayNameForTransport() const { return _sessionDisplayName; }
    virtual void maybeUpdateSessionDisplayNameFromTransport(const QString& sessionDisplayName) { } // No-op in AvatarMixer

    // Body scale
    float _targetScale;
    float _domainMinimumHeight { MIN_AVATAR_HEIGHT };
    float _domainMaximumHeight { MAX_AVATAR_HEIGHT };

    //  Hand state (are we grabbing something or not)
    char _handState;

    QVector<JointData> _jointData; ///< the state of the skeleton joints
    QVector<JointData> _lastSentJointData; ///< the state of the skeleton joints last time we transmitted
    mutable QReadWriteLock _jointDataLock;

    // key state
    KeyState _keyState;

    bool _forceFaceTrackerConnected;
    bool _hasNewJointData { true }; // set in AvatarData, cleared in Avatar

    mutable HeadData* _headData { nullptr };

    QUrl _skeletonModelURL;
    QUrl _skeletonFBXURL;
    QVector<AttachmentData> _attachmentData;
    QVector<AttachmentData> _oldAttachmentData;
    QString _displayName;
    QString _sessionDisplayName { };
    bool _lookAtSnappingEnabled { true };

    QHash<QString, int> _fstJointIndices; ///< 1-based, since zero is returned for missing keys
    QStringList _fstJointNames; ///< in order of depth-first traversal

    quint64 _errorLogExpiry; ///< time in future when to log an error

    QWeakPointer<Node> _owningAvatarMixer;

    /// Loads the joint indices, names from the FST file (if any)
    virtual void updateJointMappings();

    glm::vec3 _targetVelocity;

    SimpleMovingAverage _averageBytesReceived;

    // During recording, this holds the starting position, orientation & scale of the recorded avatar
    // During playback, it holds the origin from which to play the relative positions in the clip
    TransformPointer _recordingBasis;

    // _globalPosition is sent along with localPosition + parent because the avatar-mixer doesn't know
    // where Entities are located.  This is currently only used by the mixer to decide how often to send
    // updates about one avatar to another.
    glm::vec3 _globalPosition { 0, 0, 0 };


    quint64 _globalPositionChanged { 0 };
    quint64 _avatarBoundingBoxChanged { 0 };
    quint64 _avatarScaleChanged { 0 };
    quint64 _sensorToWorldMatrixChanged { 0 };
    quint64 _additionalFlagsChanged { 0 };
    quint64 _parentChanged { 0 };

    quint64  _lastToByteArray { 0 }; // tracks the last time we did a toByteArray

    // Some rate data for incoming data in bytes
    RateCounter<> _parseBufferRate;
    RateCounter<> _globalPositionRate;
    RateCounter<> _localPositionRate;
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

    glm::vec3 _globalBoundingBoxDimensions;
    glm::vec3 _globalBoundingBoxOffset;

    mutable ReadWriteLockable _avatarEntitiesLock;
    AvatarEntityIDs _avatarEntityDetached; // recently detached from this avatar
    AvatarEntityIDs _avatarEntityForRecording; // create new entities id for avatar recording
    AvatarEntityMap _avatarEntityData;
    bool _avatarEntityDataChanged { false };

    // used to transform any sensor into world space, including the _hmdSensorMat, or hand controllers.
    ThreadSafeValueCache<glm::mat4> _sensorToWorldMatrixCache { glm::mat4() };
    ThreadSafeValueCache<glm::mat4> _controllerLeftHandMatrixCache { glm::mat4() };
    ThreadSafeValueCache<glm::mat4> _controllerRightHandMatrixCache { glm::mat4() };

    ThreadSafeValueCache<glm::mat4> _farGrabRightMatrixCache { glm::mat4() };
    ThreadSafeValueCache<glm::mat4> _farGrabLeftMatrixCache { glm::mat4() };
    ThreadSafeValueCache<glm::mat4> _farGrabMouseMatrixCache { glm::mat4() };

    ThreadSafeValueCache<QVariantMap> _collisionCapsuleCache{ QVariantMap() };

    int getFauxJointIndex(const QString& name) const;

    float _audioLoudness { 0.0f };
    quint64 _audioLoudnessChanged { 0 };
    float _audioAverageLoudness { 0.0f };

    bool _identityDataChanged { false };
    udt::SequenceNumber _identitySequenceNumber { 0 };
    bool _hasProcessedFirstIdentity { false };
    float _density;
    int _replicaIndex { 0 };

    // null unless MyAvatar or ScriptableAvatar sending traits data to mixer
    std::unique_ptr<ClientTraitsHandler> _clientTraitsHandler;

    template <typename T, typename F>
    T readLockWithNamedJointIndex(const QString& name, const T& defaultValue, F f) const {
        int index = getFauxJointIndex(name);
        QReadLocker readLock(&_jointDataLock);
        if (index == -1) {
            index = _fstJointIndices.value(name) - 1;
        }

        // The first conditional is superfluous, but illsutrative
        if (index == -1 || index < _jointData.size()) {
            return defaultValue;
        }

        return f(index);
    }

    template <typename T, typename F>
    T readLockWithNamedJointIndex(const QString& name, F f) const {
        return readLockWithNamedJointIndex(name, T(), f);
    }

    template <typename F>
    void writeLockWithNamedJointIndex(const QString& name, F f) {
        int index = getFauxJointIndex(name);
        QWriteLocker writeLock(&_jointDataLock);
        if (index == -1) {
            index = _fstJointIndices.value(name) - 1;
        }
        if (index == -1) {
            return;
        }
        if (_jointData.size() <= index) {
            _jointData.resize(index + 1);
        }
        f(index);
    }

private:
    friend void avatarStateFromFrame(const QByteArray& frameData, AvatarData* _avatar);
    static QUrl _defaultFullAvatarModelUrl;
    // privatize the copy constructor and assignment operator so they cannot be called
    AvatarData(const AvatarData&);
    AvatarData& operator= (const AvatarData&);
};
Q_DECLARE_METATYPE(AvatarData*)

QJsonValue toJsonValue(const JointData& joint);
JointData jointDataFromJsonValue(const QJsonValue& q);

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

    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

    QVariant toVariant() const;
    bool fromVariant(const QVariant& variant);
};

QDataStream& operator<<(QDataStream& out, const AttachmentData& attachment);
QDataStream& operator>>(QDataStream& in, AttachmentData& attachment);

Q_DECLARE_METATYPE(AttachmentData)
Q_DECLARE_METATYPE(QVector<AttachmentData>)

/// Scriptable wrapper for attachments.
class AttachmentDataObject : public QObject, protected QScriptable {
    Q_OBJECT
    Q_PROPERTY(QString modelURL READ getModelURL WRITE setModelURL)
    Q_PROPERTY(QString jointName READ getJointName WRITE setJointName)
    Q_PROPERTY(glm::vec3 translation READ getTranslation WRITE setTranslation)
    Q_PROPERTY(glm::quat rotation READ getRotation WRITE setRotation)
    Q_PROPERTY(float scale READ getScale WRITE setScale)
    Q_PROPERTY(bool isSoft READ getIsSoft WRITE setIsSoft)

public:

    Q_INVOKABLE void setModelURL(const QString& modelURL);
    Q_INVOKABLE QString getModelURL() const;

    Q_INVOKABLE void setJointName(const QString& jointName);
    Q_INVOKABLE QString getJointName() const;

    Q_INVOKABLE void setTranslation(const glm::vec3& translation);
    Q_INVOKABLE glm::vec3 getTranslation() const;

    Q_INVOKABLE void setRotation(const glm::quat& rotation);
    Q_INVOKABLE glm::quat getRotation() const;

    Q_INVOKABLE void setScale(float scale);
    Q_INVOKABLE float getScale() const;

    Q_INVOKABLE void setIsSoft(bool scale);
    Q_INVOKABLE bool getIsSoft() const;
};

void registerAvatarTypes(QScriptEngine* engine);

class RayToAvatarIntersectionResult {
public:
    bool intersects { false };
    QUuid avatarID;
    float distance { FLT_MAX };
    BoxFace face;
    glm::vec3 intersection;
    glm::vec3 surfaceNormal;
    QVariantMap extraInfo;
};
Q_DECLARE_METATYPE(RayToAvatarIntersectionResult)
QScriptValue RayToAvatarIntersectionResultToScriptValue(QScriptEngine* engine, const RayToAvatarIntersectionResult& results);
void RayToAvatarIntersectionResultFromScriptValue(const QScriptValue& object, RayToAvatarIntersectionResult& results);

class ParabolaToAvatarIntersectionResult {
public:
    bool intersects { false };
    QUuid avatarID;
    float distance { 0.0f };
    float parabolicDistance { 0.0f };
    BoxFace face;
    glm::vec3 intersection;
    glm::vec3 surfaceNormal;
    QVariantMap extraInfo;
};

Q_DECLARE_METATYPE(AvatarEntityMap)

QScriptValue AvatarEntityMapToScriptValue(QScriptEngine* engine, const AvatarEntityMap& value);
void AvatarEntityMapFromScriptValue(const QScriptValue& object, AvatarEntityMap& value);

// faux joint indexes (-1 means invalid)
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


#endif // hifi_AvatarData_h
