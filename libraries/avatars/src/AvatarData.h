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

#include <CollisionInfo.h>
#include <NLPacket.h>
#include <Node.h>
#include <RegisteredMetaTypes.h>
#include <SimpleMovingAverage.h>

#include "AABox.h"
#include "HandData.h"
#include "HeadData.h"
#include "Player.h"
#include "Recorder.h"
#include "Referential.h"

typedef std::shared_ptr<AvatarData> AvatarSharedPointer;
typedef std::weak_ptr<AvatarData> AvatarWeakPointer;
typedef QHash<QUuid, AvatarSharedPointer> AvatarHash;

// avatar motion behaviors
const quint32 AVATAR_MOTION_KEYBOARD_MOTOR_ENABLED = 1U << 0;
const quint32 AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED = 1U << 1;

const quint32 AVATAR_MOTION_DEFAULTS =
        AVATAR_MOTION_KEYBOARD_MOTOR_ENABLED |
        AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED;

// these bits will be expanded as features are exposed
const quint32 AVATAR_MOTION_SCRIPTABLE_BITS =
        AVATAR_MOTION_SCRIPTED_MOTOR_ENABLED;

const qint64 AVATAR_SILENCE_THRESHOLD_USECS = 5 * USECS_PER_SECOND;

// Bitset of state flags - we store the key state, hand state, faceshift, chat circling, and existance of
// referential data in this bit set. The hand state is an octal, but is split into two sections to maintain
// backward compatibility. The bits are ordered as such (0-7 left to right).
//     +-----+-----+-+-+-+--+
//     |K0,K1|H0,H1|F|C|R|H2|
//     +-----+-----+-+-+-+--+
// Key state - K0,K1 is found in the 1st and 2nd bits
// Hand state - H0,H1,H2 is found in the 3rd, 4th, and 8th bits
// Faceshift - F is found in the 5th bit
// Chat Circling - C is found in the 6th bit
// Referential Data - R is found in the 7th bit
const int KEY_STATE_START_BIT = 0; // 1st and 2nd bits
const int HAND_STATE_START_BIT = 2; // 3rd and 4th bits
const int IS_FACESHIFT_CONNECTED = 4; // 5th bit
const int UNUSED_AVATAR_STATE_BIT_5 = 5; // 6th bit (was CHAT_CIRCLING)
const int HAS_REFERENTIAL = 6; // 7th bit
const int HAND_STATE_FINGER_POINTING_BIT = 7; // 8th bit

const char HAND_STATE_NULL = 0;
const char LEFT_HAND_POINTING_FLAG = 1;
const char RIGHT_HAND_POINTING_FLAG = 2;
const char IS_FINGER_POINTING_FLAG = 4;

static const float MAX_AVATAR_SCALE = 1000.0f;
static const float MIN_AVATAR_SCALE = .005f;

const float MAX_AUDIO_LOUDNESS = 1000.0; // close enough for mouth animation

const int AVATAR_IDENTITY_PACKET_SEND_INTERVAL_MSECS = 1000;
const int AVATAR_BILLBOARD_PACKET_SEND_INTERVAL_MSECS = 5000;

const QUrl DEFAULT_HEAD_MODEL_URL = QUrl("http://public.highfidelity.io/models/heads/defaultAvatar_head.fst");
const QUrl DEFAULT_BODY_MODEL_URL = QUrl("http://public.highfidelity.io/models/skeletons/defaultAvatar_body.fst");
const QUrl DEFAULT_FULL_AVATAR_MODEL_URL = QUrl("http://public.highfidelity.io/marketplace/contents/029db3d4-da2c-4cb2-9c08-b9612ba576f5/02949063e7c4aed42ad9d1a58461f56d.fst");

const QString DEFAULT_HEAD_MODEL_NAME = QString("Robot");
const QString DEFAULT_BODY_MODEL_NAME = QString("Robot");
const QString DEFAULT_FULL_AVATAR_MODEL_NAME = QString("Default");


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
class JointData;

class AvatarData : public QObject {
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

    Q_PROPERTY(float audioLoudness READ getAudioLoudness WRITE setAudioLoudness)
    Q_PROPERTY(float audioAverageLoudness READ getAudioAverageLoudness WRITE setAudioAverageLoudness)

    Q_PROPERTY(QString displayName READ getDisplayName WRITE setDisplayName)
    Q_PROPERTY(QString faceModelURL READ getFaceModelURLFromScript WRITE setFaceModelURLFromScript)
    Q_PROPERTY(QString skeletonModelURL READ getSkeletonModelURLFromScript WRITE setSkeletonModelURLFromScript)
    Q_PROPERTY(QVector<AttachmentData> attachmentData READ getAttachmentData WRITE setAttachmentData)
    Q_PROPERTY(QString billboardURL READ getBillboardURL WRITE setBillboardFromURL)

    Q_PROPERTY(QStringList jointNames READ getJointNames)

    Q_PROPERTY(QUuid sessionUUID READ getSessionUUID)
public:
    AvatarData();
    virtual ~AvatarData();
    
    virtual bool isMyAvatar() const { return false; }

    const QUuid& getSessionUUID() const { return _sessionUUID; }

    const glm::vec3& getPosition() const;
    virtual void setPosition(const glm::vec3 position, bool overideReferential = false);

    glm::vec3 getHandPosition() const;
    void setHandPosition(const glm::vec3& handPosition);

    virtual QByteArray toByteArray();

    /// \return true if an error should be logged
    bool shouldLogError(const quint64& now);

    /// \param packet byte array of data
    /// \param offset number of bytes into packet where data starts
    /// \return number of bytes parsed
    virtual int parseDataFromBuffer(const QByteArray& buffer);

    //  Body Rotation (degrees)
    float getBodyYaw() const { return _bodyYaw; }
    void setBodyYaw(float bodyYaw) { _bodyYaw = bodyYaw; }
    float getBodyPitch() const { return _bodyPitch; }
    void setBodyPitch(float bodyPitch) { _bodyPitch = bodyPitch; }
    float getBodyRoll() const { return _bodyRoll; }
    void setBodyRoll(float bodyRoll) { _bodyRoll = bodyRoll; }

    glm::quat getOrientation() const;
    void setOrientation(const glm::quat& orientation, bool overideReferential = false);

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
    void setTargetScale(float targetScale, bool overideReferential = false);
    void setClampedTargetScale(float targetScale, bool overideReferential = false);

    //  Hand State
    Q_INVOKABLE void setHandState(char s) { _handState = s; }
    Q_INVOKABLE char getHandState() const { return _handState; }

    const QVector<JointData>& getJointData() const { return _jointData; }
    void setJointData(const QVector<JointData>& jointData) { _jointData = jointData; }

    Q_INVOKABLE virtual void setJointData(int index, const glm::quat& rotation);
    Q_INVOKABLE virtual void clearJointData(int index);
    Q_INVOKABLE bool isJointDataValid(int index) const;
    Q_INVOKABLE virtual glm::quat getJointRotation(int index) const;

    Q_INVOKABLE void setJointData(const QString& name, const glm::quat& rotation);
    Q_INVOKABLE void clearJointData(const QString& name);
    Q_INVOKABLE bool isJointDataValid(const QString& name) const;
    Q_INVOKABLE glm::quat getJointRotation(const QString& name) const;
    
    Q_INVOKABLE virtual QVector<glm::quat> getJointRotations() const;
    Q_INVOKABLE virtual void setJointRotations(QVector<glm::quat> jointRotations);
    
    Q_INVOKABLE virtual void clearJointsData();
    
    /// Returns the index of the joint with the specified name, or -1 if not found/unknown.
    Q_INVOKABLE virtual int getJointIndex(const QString& name) const { return _jointIndices.value(name) - 1; }

    Q_INVOKABLE virtual QStringList getJointNames() const { return _jointNames; }

    Q_INVOKABLE void setBlendshape(QString name, float val) { _headData->setBlendshape(name, val); }

    void setForceFaceTrackerConnected(bool connected) { _forceFaceTrackerConnected = connected; }

    // key state
    void setKeyState(KeyState s) { _keyState = s; }
    KeyState keyState() const { return _keyState; }

    const HeadData* getHeadData() const { return _headData; }
    const HandData* getHandData() const { return _handData; }

    virtual bool findSphereCollisions(const glm::vec3& particleCenter, float particleRadius, CollisionList& collisions) {
        return false;
    }

    bool hasIdentityChangedAfterParsing(NLPacket& packet);
    QByteArray identityByteArray();
    
    bool hasBillboardChangedAfterParsing(NLPacket& packet);
    
    const QUrl& getFaceModelURL() const { return _faceModelURL; }
    QString getFaceModelURLString() const { return _faceModelURL.toString(); }
    const QUrl& getSkeletonModelURL() const { return _skeletonModelURL; }
    const QString& getDisplayName() const { return _displayName; }
    virtual void setFaceModelURL(const QUrl& faceModelURL);
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);
    
    virtual void setDisplayName(const QString& displayName);
    
    Q_INVOKABLE QVector<AttachmentData> getAttachmentData() const;
    Q_INVOKABLE virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData);
    
    Q_INVOKABLE virtual void attach(const QString& modelURL, const QString& jointName = QString(),
        const glm::vec3& translation = glm::vec3(), const glm::quat& rotation = glm::quat(), float scale = 1.0f,
        bool allowDuplicates = false, bool useSaved = true);
    
    Q_INVOKABLE void detachOne(const QString& modelURL, const QString& jointName = QString());
    Q_INVOKABLE void detachAll(const QString& modelURL, const QString& jointName = QString());
    
    virtual void setBillboard(const QByteArray& billboard);
    const QByteArray& getBillboard() const { return _billboard; }
    
    void setBillboardFromURL(const QString& billboardURL);
    const QString& getBillboardURL() { return _billboardURL; }
    
    QString getFaceModelURLFromScript() const { return _faceModelURL.toString(); }
    void setFaceModelURLFromScript(const QString& faceModelString) { setFaceModelURL(faceModelString); }
    
    QString getSkeletonModelURLFromScript() const { return _skeletonModelURL.toString(); }
    void setSkeletonModelURLFromScript(const QString& skeletonModelString) { setSkeletonModelURL(QUrl(skeletonModelString)); }
    
    void setOwningAvatarMixer(const QWeakPointer<Node>& owningAvatarMixer) { _owningAvatarMixer = owningAvatarMixer; }
    
    const AABox& getLocalAABox() const { return _localAABox; }
    const Referential* getReferential() const { return _referential; }

    int getUsecsSinceLastUpdate() const { return _averageBytesReceived.getUsecsSinceLastEvent(); }
    int getAverageBytesReceivedPerSecond() const;
    int getReceiveRate() const;

    void setVelocity(const glm::vec3 velocity) { _velocity = velocity; }
    Q_INVOKABLE glm::vec3 getVelocity() const { return _velocity; }
    const glm::vec3& getTargetVelocity() const { return _targetVelocity; }

    bool shouldDie() const { return _owningAvatarMixer.isNull() || getUsecsSinceLastUpdate() > AVATAR_SILENCE_THRESHOLD_USECS; }

public slots:
    void sendAvatarDataPacket();
    void sendIdentityPacket();
    void sendBillboardPacket();
    
    void setBillboardFromNetworkReply();
    void setJointMappingsFromNetworkReply();
    void setSessionUUID(const QUuid& sessionUUID) { _sessionUUID = sessionUUID; }
    bool hasReferential();
    
    bool isPlaying();
    bool isPaused();
    qint64 playerElapsed();
    qint64 playerLength();
    int playerCurrentFrame();
    int playerFrameNumber();
    
    void loadRecording(QString filename);
    void startPlaying();
    void setPlayerVolume(float volume);
    void setPlayerAudioOffset(int audioOffset);
    void setPlayerFrame(unsigned int frame);
    void setPlayerTime(unsigned int time);
    void setPlayFromCurrentLocation(bool playFromCurrentLocation);
    void setPlayerLoop(bool loop);
    void setPlayerUseDisplayName(bool useDisplayName);
    void setPlayerUseAttachments(bool useAttachments);
    void setPlayerUseHeadModel(bool useHeadModel);
    void setPlayerUseSkeletonModel(bool useSkeletonModel);
    void play();
    void pausePlayer();
    void stopPlaying();
    
protected:
    QUuid _sessionUUID;
    glm::vec3 _position = START_LOCATION;
    glm::vec3 _handPosition;
    
    Referential* _referential;

    //  Body rotation
    float _bodyYaw;     // degrees
    float _bodyPitch;   // degrees
    float _bodyRoll;    // degrees

    // Body scale
    float _targetScale;

    //  Hand state (are we grabbing something or not)
    char _handState;

    QVector<JointData> _jointData; ///< the state of the skeleton joints

    // key state
    KeyState _keyState;

    bool _forceFaceTrackerConnected;
    bool _hasNewJointRotations; // set in AvatarData, cleared in Avatar

    HeadData* _headData;
    HandData* _handData;

    QUrl _faceModelURL; // These need to be empty so that on first time setting them they will not short circuit
    QUrl _skeletonModelURL; // These need to be empty so that on first time setting them they will not short circuit
    QVector<AttachmentData> _attachmentData;
    QString _displayName;

    float _displayNameTargetAlpha;
    float _displayNameAlpha;

    QByteArray _billboard;
    QString _billboardURL;
    
    QHash<QString, int> _jointIndices; ///< 1-based, since zero is returned for missing keys
    QStringList _jointNames; ///< in order of depth-first traversal

    quint64 _errorLogExpiry; ///< time in future when to log an error
    
    QWeakPointer<Node> _owningAvatarMixer;
    
    PlayerPointer _player;
    
    /// Loads the joint indices, names from the FST file (if any)
    virtual void updateJointMappings();
    void changeReferential(Referential* ref);

    glm::vec3 _velocity;
    glm::vec3 _targetVelocity;

    AABox _localAABox;

    SimpleMovingAverage _averageBytesReceived;

private:
    // privatize the copy constructor and assignment operator so they cannot be called
    AvatarData(const AvatarData&);
    AvatarData& operator= (const AvatarData&);
};
Q_DECLARE_METATYPE(AvatarData*)

class JointData {
public:
    bool valid;
    glm::quat rotation;
};

class AttachmentData {
public:
    QUrl modelURL;
    QString jointName;
    glm::vec3 translation;
    glm::quat rotation;
    float scale;
    
    AttachmentData();
    
    bool isValid() const { return modelURL.isValid(); }
    
    bool operator==(const AttachmentData& other) const;
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

public:
    
    Q_INVOKABLE void setModelURL(const QString& modelURL) const;
    Q_INVOKABLE QString getModelURL() const;
    
    Q_INVOKABLE void setJointName(const QString& jointName) const;
    Q_INVOKABLE QString getJointName() const;
    
    Q_INVOKABLE void setTranslation(const glm::vec3& translation) const;
    Q_INVOKABLE glm::vec3 getTranslation() const;
    
    Q_INVOKABLE void setRotation(const glm::quat& rotation) const;
    Q_INVOKABLE glm::quat getRotation() const;
    
    Q_INVOKABLE void setScale(float scale) const;
    Q_INVOKABLE float getScale() const;
};

void registerAvatarTypes(QScriptEngine* engine);

#endif // hifi_AvatarData_h
