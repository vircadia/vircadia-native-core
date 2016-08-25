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

class QDataStream;

class AttachmentData;
class Transform;
using TransformPointer = std::shared_ptr<Transform>;

// When writing out avatarEntities to a QByteArray, if the parentID is the ID of MyAvatar, use this ID instead.  This allows
// the value to be reset when the sessionID changes.
const QUuid AVATAR_SELF_ID = QUuid("{00000000-0000-0000-0000-000000000001}");

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

    virtual QByteArray toByteArray(bool cullSmallChanges, bool sendAll);
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
        AvatarEntityMap avatarEntityData;
    };

    static void parseAvatarIdentityPacket(const QByteArray& data, Identity& identityOut);

    // returns true if identity has changed, false otherwise.
    bool processAvatarIdentity(const Identity& identity);

    QByteArray identityByteArray();

    const QUrl& getSkeletonModelURL() const { return _skeletonModelURL; }
    const QString& getDisplayName() const { return _displayName; }
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);

    virtual void setDisplayName(const QString& displayName);

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

protected:
    glm::vec3 _handPosition;

    // Body scale
    float _targetScale;

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
    glm::vec3 _globalPosition;

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


#endif // hifi_AvatarData_h
