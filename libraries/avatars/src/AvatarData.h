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
/* VS2010 defines stdint.h, but not inttypes.h */
#if defined(_MSC_VER)
typedef signed char  int8_t;
typedef signed short int16_t;
typedef signed int   int32_t;
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int   uint32_t;
typedef signed long long   int64_t;
typedef unsigned long long quint64;
#define PRId64 "I64d"
#else
#include <inttypes.h>
#endif
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
#include <QReadWriteLock>

#include <JointData.h>
#include <NLPacket.h>
#include <Node.h>
#include <RegisteredMetaTypes.h>
#include <SimpleMovingAverage.h>
#include <SpatiallyNestable.h>
#include <NumericalConstants.h>
#include <Packed.h>
#include <ThreadSafeValueCache.h>
#include <SharedUtil.h>

#include "AABox.h"
#include "HeadData.h"
#include "PathUtils.h"

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

const qint64 AVATAR_SILENCE_THRESHOLD_USECS = 5 * USECS_PER_SECOND;



// Bitset of state flags - we store the key state, hand state, Faceshift, eye tracking, and existence of
// referential data in this bit set. The hand state is an octal, but is split into two sections to maintain
// backward compatibility. The bits are ordered as such (0-7 left to right).
//     +-----+-----+-+-+-+--+
//     |K0,K1|H0,H1|F|E|R|H2|
//     +-----+-----+-+-+-+--+
// Key state - K0,K1 is found in the 1st and 2nd bits
// Hand state - H0,H1,H2 is found in the 3rd, 4th, and 8th bits
// Faceshift - F is found in the 5th bit
// Eye tracker - E is found in the 6th bit
// Referential Data - R is found in the 7th bit
const int KEY_STATE_START_BIT = 0; // 1st and 2nd bits
const int HAND_STATE_START_BIT = 2; // 3rd and 4th bits
const int IS_FACESHIFT_CONNECTED = 4; // 5th bit
const int IS_EYE_TRACKER_CONNECTED = 5; // 6th bit (was CHAT_CIRCLING)
const int HAS_REFERENTIAL = 6; // 7th bit
const int HAND_STATE_FINGER_POINTING_BIT = 7; // 8th bit


const char HAND_STATE_NULL = 0;
const char LEFT_HAND_POINTING_FLAG = 1;
const char RIGHT_HAND_POINTING_FLAG = 2;
const char IS_FINGER_POINTING_FLAG = 4;

// AvatarData state flags - we store the details about the packet encoding in the first byte, 
// before the "header" structure
const char AVATARDATA_FLAGS_MINIMUM = 0;

using smallFloat = uint16_t; // a compressed float with less precision, user defined radix

namespace AvatarDataPacket {
    // Packet State Flags - we store the details about the existence of other records in this bitset:
    // AvatarGlobalPosition, Avatar Faceshift, eye tracking, and existence of
    using HasFlags = uint16_t;
    const HasFlags PACKET_HAS_AVATAR_GLOBAL_POSITION = 1U << 0;
    const HasFlags PACKET_HAS_AVATAR_LOCAL_POSITION  = 1U << 1; // FIXME - can this be in the PARENT_INFO??
    const HasFlags PACKET_HAS_AVATAR_DIMENSIONS      = 1U << 2;
    const HasFlags PACKET_HAS_AVATAR_ORIENTATION     = 1U << 3;
    const HasFlags PACKET_HAS_AVATAR_SCALE           = 1U << 4;
    const HasFlags PACKET_HAS_LOOK_AT_POSITION       = 1U << 5;
    const HasFlags PACKET_HAS_AUDIO_LOUDNESS         = 1U << 6;
    const HasFlags PACKET_HAS_SENSOR_TO_WORLD_MATRIX = 1U << 7;
    const HasFlags PACKET_HAS_ADDITIONAL_FLAGS       = 1U << 8;
    const HasFlags PACKET_HAS_PARENT_INFO            = 1U << 9;
    const HasFlags PACKET_HAS_FACE_TRACKER_INFO      = 1U << 10;
    const HasFlags PACKET_HAS_JOINT_DATA             = 1U << 11;

    // NOTE: AvatarDataPackets start with a uint16_t sequence number that is not reflected in the Header structure.

    PACKED_BEGIN struct Header {
        HasFlags packetHasFlags;        // state flags, indicated which additional records are included in the packet
                                        //    bit 0  - has AvatarGlobalPosition
                                        //    bit 1  - has AvatarLocalPosition
                                        //    bit 2  - has AvatarDimensions
                                        //    bit 3  - has AvatarOrientation
                                        //    bit 4  - has AvatarScale
                                        //    bit 5  - has LookAtPosition
                                        //    bit 6  - has AudioLoudness
                                        //    bit 7  - has SensorToWorldMatrix
                                        //    bit 8  - has AdditionalFlags
                                        //    bit 9  - has ParentInfo
                                        //    bit 10 - has FaceTrackerInfo
                                        //    bit 11 - has JointData
    } PACKED_END;
    const size_t HEADER_SIZE = 2;

    PACKED_BEGIN struct AvatarGlobalPosition {
        float globalPosition[3];          // avatar's position
    } PACKED_END;
    const size_t AVATAR_GLOBAL_POSITION_SIZE = 12;

    PACKED_BEGIN struct AvatarLocalPosition {
        float localPosition[3];             // this appears to be the avatar local position?? 
                                          // this is a reduced precision radix
                                          // FIXME - could this be changed into compressed floats?
    } PACKED_END;
    const size_t AVATAR_LOCAL_POSITION_SIZE = 12;

    PACKED_BEGIN struct AvatarDimensions {
        float avatarDimensions[3];        // avatar's bounding box in world space units, but relative to the 
                                          // position. Assumed to be centered around the world position
                                          // FIXME - could this be changed into compressed floats?
    } PACKED_END;
    const size_t AVATAR_DIMENSIONS_SIZE = 12;


    PACKED_BEGIN struct AvatarOrientation {
        smallFloat localOrientation[3];   // avatar's local euler angles (degrees, compressed) relative to the
                                          // thing it's attached to, or world relative if not attached
    } PACKED_END;
    const size_t AVATAR_ORIENTATION_SIZE = 6;

    PACKED_BEGIN struct AvatarScale {
        smallFloat scale;                 // avatar's scale, (compressed) 'ratio' encoding uses sign bit as flag.
    } PACKED_END;
    const size_t AVATAR_SCALE_SIZE = 2;

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

    PACKED_BEGIN struct AudioLoudness {
        smallFloat audioLoudness;         // current loudness of microphone, (compressed)
    } PACKED_END;
    const size_t AUDIO_LOUDNESS_SIZE = 2;

    PACKED_BEGIN struct SensorToWorldMatrix {
        // FIXME - these 20 bytes are only used by viewers if my avatar has "attachments"
        // we could save these bytes if no attachments are active.
        //
        // POTENTIAL SAVINGS - 20 bytes

        uint8_t sensorToWorldQuat[6];     // 6 byte compressed quaternion part of sensor to world matrix
        uint16_t sensorToWorldScale;      // uniform scale of sensor to world matrix
        float sensorToWorldTrans[3];      // fourth column of sensor to world matrix
                                          // FIXME - sensorToWorldTrans might be able to be better compressed if it was
                                          // relative to the avatar position.
    } PACKED_END;
    const size_t SENSOR_TO_WORLD_SIZE = 20;

    PACKED_BEGIN struct AdditionalFlags {
        uint8_t flags;                    // additional flags: hand state, key state, eye tracking
    } PACKED_END;
    const size_t ADDITIONAL_FLAGS_SIZE = 1;

    // only present if HAS_REFERENTIAL flag is set in AvatarInfo.flags
    PACKED_BEGIN struct ParentInfo {
        uint8_t parentUUID[16];       // rfc 4122 encoded
        uint16_t parentJointIndex;
    } PACKED_END;
    const size_t PARENT_INFO_SIZE = 18;

    // only present if IS_FACESHIFT_CONNECTED flag is set in AvatarInfo.flags
    PACKED_BEGIN struct FaceTrackerInfo {
        float leftEyeBlink;
        float rightEyeBlink;
        float averageLoudness;
        float browAudioLift;
        uint8_t numBlendshapeCoefficients;
        // float blendshapeCoefficients[numBlendshapeCoefficients];
    } PACKED_END;
    const size_t FACE_TRACKER_INFO_SIZE = 17;

    // variable length structure follows
    /*
    struct JointData {
        uint8_t numJoints;
        uint8_t rotationValidityBits[ceil(numJoints / 8)];     // one bit per joint, if true then a compressed rotation follows.
        SixByteQuat rotation[numValidRotations];               // encodeded and compressed by packOrientationQuatToSixBytes()
        uint8_t translationValidityBits[ceil(numJoints / 8)];  // one bit per joint, if true then a compressed translation follows.
        SixByteTrans translation[numValidTranslations];        // encodeded and compressed by packFloatVec3ToSignedTwoByteFixed()
    };
    */

    // OLD FORMAT....
    PACKED_BEGIN struct AvatarInfo {
        // FIXME - this has 8 unqiue items, we could use a simple header byte to indicate whether or not the fields 
        // exist in the packet and have changed since last being sent.
        float globalPosition[3];          // avatar's position
                                                // FIXME - possible savings:
                                                //    a) could be encoded as relative to last known position, most movements
                                                //       will be withing a smaller radix
                                                //    b) would still need an intermittent absolute value.

        float position[3];                // skeletal model's position 
                                                // FIXME - this used to account for a registration offset from the avatar's position
                                                // to the position of the skeletal model/mesh. This relative offset doesn't change from
                                                // frame to frame, instead only changes when the model changes, it could be moved to the
                                                // identity packet and/or only included when it changes.
                                                // if it's encoded relative to the globalPosition, it could be reduced to a smaller radix
                                                //
                                                // POTENTIAL SAVINGS - 12 bytes

        float globalBoundingBoxCorner[3]; // global position of the lowest corner of the avatar's bounding box 
                                                // FIXME - this would change less frequently if it was the dimensions of the bounding box
                                                // instead of the corner.
                                                //
                                                // POTENTIAL SAVINGS - 12 bytes

        uint16_t localOrientation[3];     // avatar's local euler angles (degrees, compressed) relative to the thing it's attached to
        uint16_t scale;                   // (compressed) 'ratio' encoding uses sign bit as flag.
                                                // FIXME - this doesn't change every frame
                                                //
                                                // POTENTIAL SAVINGS - 2 bytes

        float lookAtPosition[3];          // world space position that eyes are focusing on.
                                                // FIXME - unless the person has an eye tracker, this is simulated... 
                                                //    a) maybe we can just have the client calculate this
                                                //    b) at distance this will be hard to discern and can likely be 
                                                //       descimated or dropped completely
                                                //
                                                // POTENTIAL SAVINGS - 12 bytes

        uint16_t audioLoudness;           // current loundess of microphone
                                                // FIXME - 
                                                //    a) this could probably be decimated with a smaller radix <<< DONE
                                                //    b) this doesn't change every frame
                                                //
                                                // POTENTIAL SAVINGS - 4-2 bytes

        // FIXME - these 20 bytes are only used by viewers if my avatar has "attachments"
        // we could save these bytes if no attachments are active.
        //
        // POTENTIAL SAVINGS - 20 bytes

        uint8_t sensorToWorldQuat[6];     // 6 byte compressed quaternion part of sensor to world matrix
        uint16_t sensorToWorldScale;      // uniform scale of sensor to world matrix
        float sensorToWorldTrans[3];      // fourth column of sensor to world matrix
                                                // FIXME - sensorToWorldTrans might be able to be better compressed if it was
                                                // relative to the avatar position.
        uint8_t flags;
    } PACKED_END;
    const size_t AVATAR_INFO_SIZE = 79;
}

static const float MAX_AVATAR_SCALE = 1000.0f;
static const float MIN_AVATAR_SCALE = .005f;

const float MAX_AUDIO_LOUDNESS = 1000.0f; // close enough for mouth animation

const int AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS = 1000;

// See also static AvatarData::defaultFullAvatarModelUrl().
const QString DEFAULT_FULL_AVATAR_MODEL_NAME = QString("Default");

// how often should we send a full report about joint rotations, even if they haven't changed?
const float AVATAR_SEND_FULL_UPDATE_RATIO = 0.02f;
// this controls how large a change in joint-rotation must be before the interface sends it to the avatar mixer
const float AVATAR_MIN_ROTATION_DOT = 0.9999999f;
const float AVATAR_MIN_TRANSLATION = 0.0001f;


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

class AvatarData : public QObject, public SpatiallyNestable {
    Q_OBJECT

    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(float scale READ getTargetScale WRITE setTargetScale)
    Q_PROPERTY(glm::vec3 handPosition READ getHandPosition WRITE setHandPosition)
    Q_PROPERTY(float bodyYaw READ getBodyYaw WRITE setBodyYaw)
    Q_PROPERTY(float bodyPitch READ getBodyPitch WRITE setBodyPitch)
    Q_PROPERTY(float bodyRoll READ getBodyRoll WRITE setBodyRoll)

    Q_PROPERTY(glm::quat orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(glm::quat headOrientation READ getHeadOrientation WRITE setHeadOrientation)
    Q_PROPERTY(float headPitch READ getHeadPitch WRITE setHeadPitch)
    Q_PROPERTY(float headYaw READ getHeadYaw WRITE setHeadYaw)
    Q_PROPERTY(float headRoll READ getHeadRoll WRITE setHeadRoll)

    Q_PROPERTY(glm::vec3 velocity READ getVelocity WRITE setVelocity)
    Q_PROPERTY(glm::vec3 angularVelocity READ getAngularVelocity WRITE setAngularVelocity)

    Q_PROPERTY(float audioLoudness READ getAudioLoudness WRITE setAudioLoudness)
    Q_PROPERTY(float audioAverageLoudness READ getAudioAverageLoudness WRITE setAudioAverageLoudness)

    Q_PROPERTY(QString displayName READ getDisplayName WRITE setDisplayName)
    // sessionDisplayName is sanitized, defaulted version displayName that is defined by the AvatarMixer rather than by Interface clients.
    // The result is unique among all avatars present at the time.
    Q_PROPERTY(QString sessionDisplayName READ getSessionDisplayName WRITE setSessionDisplayName)
    Q_PROPERTY(QString skeletonModelURL READ getSkeletonModelURLFromScript WRITE setSkeletonModelURLFromScript)
    Q_PROPERTY(QVector<AttachmentData> attachmentData READ getAttachmentData WRITE setAttachmentData)

    Q_PROPERTY(QStringList jointNames READ getJointNames)

    Q_PROPERTY(QUuid sessionUUID READ getSessionUUID)

    Q_PROPERTY(glm::mat4 sensorToWorldMatrix READ getSensorToWorldMatrix)
    Q_PROPERTY(glm::mat4 controllerLeftHandMatrix READ getControllerLeftHandMatrix)
    Q_PROPERTY(glm::mat4 controllerRightHandMatrix READ getControllerRightHandMatrix)

public:

    static const QString FRAME_NAME;

    static void fromFrame(const QByteArray& frameData, AvatarData& avatar);
    static QByteArray toFrame(const AvatarData& avatar);

    AvatarData();
    virtual ~AvatarData();

    static const QUrl& defaultFullAvatarModelUrl();

    virtual bool isMyAvatar() const { return false; }

    const QUuid getSessionUUID() const { return getID(); }

    glm::vec3 getHandPosition() const;
    void setHandPosition(const glm::vec3& handPosition);

    typedef enum { 
        MinimumData, 
        CullSmallData,
        IncludeSmallData,
        SendAllData
    } AvatarDataDetail;

    virtual QByteArray toByteArray(AvatarDataDetail dataDetail);
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

    using SpatiallyNestable::setPosition;
    virtual void setPosition(const glm::vec3& position) override;
    using SpatiallyNestable::setOrientation;
    virtual void setOrientation(const glm::quat& orientation) override;

    void nextAttitude(glm::vec3 position, glm::quat orientation); // Can be safely called at any time.
    virtual void updateAttitude() {} // Tell skeleton mesh about changes

    glm::quat getHeadOrientation() const { return _headData->getOrientation(); }
    void setHeadOrientation(const glm::quat& orientation) { _headData->setOrientation(orientation); }

    // access to Head().set/getMousePitch (degrees)
    float getHeadPitch() const { return _headData->getBasePitch(); }
    void setHeadPitch(float value) { _headData->setBasePitch(value); }

    float getHeadYaw() const { return _headData->getBaseYaw(); }
    void setHeadYaw(float value) { _headData->setBaseYaw(value); }

    float getHeadRoll() const { return _headData->getBaseRoll(); }
    void setHeadRoll(float value) { _headData->setBaseRoll(value); }

    // access to Head().set/getAverageLoudness
    float getAudioLoudness() const { return _headData->getAudioLoudness(); }
    void setAudioLoudness(float value) { _headData->setAudioLoudness(value); }
    float getAudioAverageLoudness() const { return _headData->getAudioAverageLoudness(); }
    void setAudioAverageLoudness(float value) { _headData->setAudioAverageLoudness(value); }

    //  Scale
    float getTargetScale() const;
    void setTargetScale(float targetScale);
    void setTargetScaleVerbose(float targetScale);

    float getDomainLimitedScale() const { return glm::clamp(_targetScale, _domainMinimumScale, _domainMaximumScale); }
    void setDomainMinimumScale(float domainMinimumScale)
        { _domainMinimumScale = glm::clamp(domainMinimumScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE); }
    void setDomainMaximumScale(float domainMaximumScale)
        { _domainMaximumScale = glm::clamp(domainMaximumScale, MIN_AVATAR_SCALE, MAX_AVATAR_SCALE); }

    //  Hand State
    Q_INVOKABLE void setHandState(char s) { _handState = s; }
    Q_INVOKABLE char getHandState() const { return _handState; }

    const QVector<JointData>& getRawJointData() const { return _jointData; }
    Q_INVOKABLE void setRawJointData(QVector<JointData> data);

    Q_INVOKABLE virtual void setJointData(int index, const glm::quat& rotation, const glm::vec3& translation);
    Q_INVOKABLE virtual void setJointRotation(int index, const glm::quat& rotation);
    Q_INVOKABLE virtual void setJointTranslation(int index, const glm::vec3& translation);
    Q_INVOKABLE virtual void clearJointData(int index);
    Q_INVOKABLE bool isJointDataValid(int index) const;
    Q_INVOKABLE virtual glm::quat getJointRotation(int index) const;
    Q_INVOKABLE virtual glm::vec3 getJointTranslation(int index) const;

    Q_INVOKABLE void setJointData(const QString& name, const glm::quat& rotation, const glm::vec3& translation);
    Q_INVOKABLE void setJointRotation(const QString& name, const glm::quat& rotation);
    Q_INVOKABLE void setJointTranslation(const QString& name, const glm::vec3& translation);
    Q_INVOKABLE void clearJointData(const QString& name);
    Q_INVOKABLE bool isJointDataValid(const QString& name) const;
    Q_INVOKABLE glm::quat getJointRotation(const QString& name) const;
    Q_INVOKABLE glm::vec3 getJointTranslation(const QString& name) const;

    Q_INVOKABLE virtual QVector<glm::quat> getJointRotations() const;
    Q_INVOKABLE virtual void setJointRotations(QVector<glm::quat> jointRotations);
    Q_INVOKABLE virtual void setJointTranslations(QVector<glm::vec3> jointTranslations);

    Q_INVOKABLE virtual void clearJointsData();

    /// Returns the index of the joint with the specified name, or -1 if not found/unknown.
    Q_INVOKABLE virtual int getJointIndex(const QString& name) const;

    Q_INVOKABLE virtual QStringList getJointNames() const;

    Q_INVOKABLE void setBlendshape(QString name, float val) { _headData->setBlendshape(name, val); }

    Q_INVOKABLE QVariantList getAttachmentsVariant() const;
    Q_INVOKABLE void setAttachmentsVariant(const QVariantList& variant);

    Q_INVOKABLE void updateAvatarEntity(const QUuid& entityID, const QByteArray& entityData);
    Q_INVOKABLE void clearAvatarEntity(const QUuid& entityID);

    void setForceFaceTrackerConnected(bool connected) { _forceFaceTrackerConnected = connected; }

    // key state
    void setKeyState(KeyState s) { _keyState = s; }
    KeyState keyState() const { return _keyState; }

    const HeadData* getHeadData() const { return _headData; }

    struct Identity {
        QUuid uuid;
        QUrl skeletonModelURL;
        QVector<AttachmentData> attachmentData;
        QString displayName;
        QString sessionDisplayName;
        AvatarEntityMap avatarEntityData;
    };

    static void parseAvatarIdentityPacket(const QByteArray& data, Identity& identityOut);

    // returns true if identity has changed, false otherwise.
    bool processAvatarIdentity(const Identity& identity);

    QByteArray identityByteArray();

    const QUrl& getSkeletonModelURL() const { return _skeletonModelURL; }
    const QString& getDisplayName() const { return _displayName; }
    const QString& getSessionDisplayName() const { return _sessionDisplayName; }
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);

    virtual void setDisplayName(const QString& displayName);
    virtual void setSessionDisplayName(const QString& sessionDisplayName) { _sessionDisplayName = sessionDisplayName; };

    Q_INVOKABLE QVector<AttachmentData> getAttachmentData() const;
    Q_INVOKABLE virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData);

    Q_INVOKABLE virtual void attach(const QString& modelURL, const QString& jointName = QString(),
                                    const glm::vec3& translation = glm::vec3(), const glm::quat& rotation = glm::quat(),
                                    float scale = 1.0f, bool isSoft = false,
                                    bool allowDuplicates = false, bool useSaved = true);

    Q_INVOKABLE void detachOne(const QString& modelURL, const QString& jointName = QString());
    Q_INVOKABLE void detachAll(const QString& modelURL, const QString& jointName = QString());

    QString getSkeletonModelURLFromScript() const { return _skeletonModelURL.toString(); }
    void setSkeletonModelURLFromScript(const QString& skeletonModelString) { setSkeletonModelURL(QUrl(skeletonModelString)); }

    void setOwningAvatarMixer(const QWeakPointer<Node>& owningAvatarMixer) { _owningAvatarMixer = owningAvatarMixer; }

    const AABox& getLocalAABox() const { return _localAABox; }

    int getUsecsSinceLastUpdate() const { return _averageBytesReceived.getUsecsSinceLastEvent(); }
    int getAverageBytesReceivedPerSecond() const;
    int getReceiveRate() const;

    const glm::vec3& getTargetVelocity() const { return _targetVelocity; }

    bool shouldDie() const { return _owningAvatarMixer.isNull() || getUsecsSinceLastUpdate() > AVATAR_SILENCE_THRESHOLD_USECS; }

    void clearRecordingBasis();
    TransformPointer getRecordingBasis() const;
    void setRecordingBasis(TransformPointer recordingBasis = TransformPointer());
    QJsonObject toJson() const;
    void fromJson(const QJsonObject& json);

    glm::vec3 getClientGlobalPosition() { return _globalPosition; }
    glm::vec3 getGlobalBoundingBoxCorner() { return _globalBoundingBoxCorner; }

    Q_INVOKABLE AvatarEntityMap getAvatarEntityData() const;
    Q_INVOKABLE void setAvatarEntityData(const AvatarEntityMap& avatarEntityData);
    void setAvatarEntityDataChanged(bool value) { _avatarEntityDataChanged = value; }
    AvatarEntityIDs getAndClearRecentlyDetachedIDs();

    // thread safe
    Q_INVOKABLE glm::mat4 getSensorToWorldMatrix() const;
    Q_INVOKABLE glm::mat4 getControllerLeftHandMatrix() const;
    Q_INVOKABLE glm::mat4 getControllerRightHandMatrix() const;

public slots:
    void sendAvatarDataPacket();
    void sendIdentityPacket();

    void setJointMappingsFromNetworkReply();
    void setSessionUUID(const QUuid& sessionUUID) { setID(sessionUUID); }

    virtual glm::quat getAbsoluteJointRotationInObjectFrame(int index) const override;
    virtual glm::vec3 getAbsoluteJointTranslationInObjectFrame(int index) const override;
    virtual bool setAbsoluteJointRotationInObjectFrame(int index, const glm::quat& rotation) override { return false; }
    virtual bool setAbsoluteJointTranslationInObjectFrame(int index, const glm::vec3& translation) override { return false; }

    float getTargetScale() { return _targetScale; }

protected:
    void lazyInitHeadData();

    bool avatarLocalPositionChanged();
    bool avatarDimensionsChanged();
    bool avatarOrientationChanged();
    bool avatarScaleChanged();
    bool lookAtPositionChanged();
    bool audioLoudnessChanged();
    bool sensorToWorldMatrixChanged();
    bool additionalFlagsChanged();

    bool hasParent() { return !getParentID().isNull(); }
    bool parentInfoChanged();

    bool hasFaceTracker() { return _headData ? _headData->_isFaceTrackerConnected : false; }
    bool faceTrackerInfoChanged();

    QByteArray toByteArray_OLD(AvatarDataDetail dataDetail);
    QByteArray toByteArray_NEW(AvatarDataDetail dataDetail);

    int parseDataFromBuffer_OLD(const QByteArray& buffer);
    int parseDataFromBuffer_NEW(const QByteArray& buffer);

    glm::vec3 _handPosition;
    virtual const QString& getSessionDisplayNameForTransport() const { return _sessionDisplayName; }
    virtual void maybeUpdateSessionDisplayNameFromTransport(const QString& sessionDisplayName) { } // No-op in AvatarMixer

    // Body scale
    float _targetScale;
    float _domainMinimumScale { MIN_AVATAR_SCALE };
    float _domainMaximumScale { MAX_AVATAR_SCALE };

    //  Hand state (are we grabbing something or not)
    char _handState;

    QVector<JointData> _jointData; ///< the state of the skeleton joints
    QVector<JointData> _lastSentJointData; ///< the state of the skeleton joints last time we transmitted
    mutable QReadWriteLock _jointDataLock;

    // key state
    KeyState _keyState;

    bool _forceFaceTrackerConnected;
    bool _hasNewJointRotations; // set in AvatarData, cleared in Avatar
    bool _hasNewJointTranslations; // set in AvatarData, cleared in Avatar

    HeadData* _headData;

    QUrl _skeletonModelURL;
    bool _firstSkeletonCheck { true };
    QUrl _skeletonFBXURL;
    QVector<AttachmentData> _attachmentData;
    QString _displayName;
    QString _sessionDisplayName { };
    const QUrl& cannonicalSkeletonModelURL(const QUrl& empty);

    float _displayNameTargetAlpha;
    float _displayNameAlpha;

    QHash<QString, int> _jointIndices; ///< 1-based, since zero is returned for missing keys
    QStringList _jointNames; ///< in order of depth-first traversal

    quint64 _errorLogExpiry; ///< time in future when to log an error

    QWeakPointer<Node> _owningAvatarMixer;

    /// Loads the joint indices, names from the FST file (if any)
    virtual void updateJointMappings();

    glm::vec3 _targetVelocity;

    AABox _localAABox;

    SimpleMovingAverage _averageBytesReceived;

    // During recording, this holds the starting position, orientation & scale of the recorded avatar
    // During playback, it holds the origin from which to play the relative positions in the clip
    TransformPointer _recordingBasis;

    // _globalPosition is sent along with localPosition + parent because the avatar-mixer doesn't know
    // where Entities are located.  This is currently only used by the mixer to decide how often to send
    // updates about one avatar to another.
    glm::vec3 _globalPosition { 0, 0, 0 };

    glm::vec3 _lastSentGlobalPosition { 0, 0, 0 };
    glm::vec3 _lastSentLocalPosition { 0, 0, 0 };
    glm::vec3 _lastSentAvatarDimensions { 0, 0, 0 };
    glm::quat _lastSentLocalOrientation;
    float _lastSentScale { 0 };
    glm::vec3 _lastSentLookAt { 0, 0, 0 };
    float _lastSentAudioLoudness { 0 };
    glm::mat4 _lastSentSensorToWorldMatrix;
    uint8_t _lastSentAdditionalFlags { 0 };
    QUuid _lastSentParentID;
    quint16 _lastSentParentJointIndex { 0 };
   

    glm::vec3 _globalBoundingBoxCorner;

    mutable ReadWriteLockable _avatarEntitiesLock;
    AvatarEntityIDs _avatarEntityDetached; // recently detached from this avatar
    AvatarEntityMap _avatarEntityData;
    bool _avatarEntityDataLocallyEdited { false };
    bool _avatarEntityDataChanged { false };

    // used to transform any sensor into world space, including the _hmdSensorMat, or hand controllers.
    ThreadSafeValueCache<glm::mat4> _sensorToWorldMatrixCache { glm::mat4() };
    ThreadSafeValueCache<glm::mat4> _controllerLeftHandMatrixCache { glm::mat4() };
    ThreadSafeValueCache<glm::mat4> _controllerRightHandMatrixCache { glm::mat4() };

    int getFauxJointIndex(const QString& name) const;

    AvatarDataPacket::AvatarInfo _lastAvatarInfo;
    glm::mat4 _lastSensorToWorldMatrix;

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
    void fromVariant(const QVariant& variant);
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
RayToAvatarIntersectionResult() : intersects(false), avatarID(), distance(0) {}
    bool intersects;
    QUuid avatarID;
    float distance;
    glm::vec3 intersection;
};

Q_DECLARE_METATYPE(RayToAvatarIntersectionResult)

QScriptValue RayToAvatarIntersectionResultToScriptValue(QScriptEngine* engine, const RayToAvatarIntersectionResult& results);
void RayToAvatarIntersectionResultFromScriptValue(const QScriptValue& object, RayToAvatarIntersectionResult& results);

// faux joint indexes (-1 means invalid)
const int SENSOR_TO_WORLD_MATRIX_INDEX = 65534; // -2
const int CONTROLLER_RIGHTHAND_INDEX = 65533; // -3
const int CONTROLLER_LEFTHAND_INDEX = 65532; // -4
const int CAMERA_RELATIVE_CONTROLLER_RIGHTHAND_INDEX = 65531; // -5
const int CAMERA_RELATIVE_CONTROLLER_LEFTHAND_INDEX = 65530; // -6

#endif // hifi_AvatarData_h
