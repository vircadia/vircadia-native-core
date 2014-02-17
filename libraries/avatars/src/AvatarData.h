//
//  AvatarData.h
//  hifi
//
//  Created by Stephen Birarda on 4/9/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__AvatarData__
#define __hifi__AvatarData__

#include <string>
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

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QUuid>
#include <QtCore/QVariantMap>

#include <CollisionInfo.h>
#include <RegisteredMetaTypes.h>
#include <NodeData.h>

#include "HeadData.h"
#include "HandData.h"

// First bitset
const int KEY_STATE_START_BIT = 0; // 1st and 2nd bits
const int HAND_STATE_START_BIT = 2; // 3rd and 4th bits
const int IS_FACESHIFT_CONNECTED = 4; // 5th bit
const int IS_CHAT_CIRCLING_ENABLED = 5;

static const float MAX_AVATAR_SCALE = 1000.f;
static const float MIN_AVATAR_SCALE = .005f;

const float MAX_AUDIO_LOUDNESS = 1000.0; // close enough for mouth animation

const QUrl DEFAULT_HEAD_MODEL_URL = QUrl("http://public.highfidelity.io/meshes/defaultAvatar_head.fst");
const QUrl DEFAULT_BODY_MODEL_URL = QUrl("http://public.highfidelity.io/meshes/defaultAvatar_body.fst");

enum KeyState {
    NO_KEY_DOWN = 0,
    INSERT_KEY_DOWN,
    DELETE_KEY_DOWN
};

const glm::vec3 vec3Zero(0.0f);

class AvatarData : public NodeData {
    Q_OBJECT

    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(float scale READ getTargetScale WRITE setTargetScale)
    Q_PROPERTY(glm::vec3 handPosition READ getHandPosition WRITE setHandPosition)
    Q_PROPERTY(float bodyYaw READ getBodyYaw WRITE setBodyYaw)
    Q_PROPERTY(float bodyPitch READ getBodyPitch WRITE setBodyPitch)
    Q_PROPERTY(float bodyRoll READ getBodyRoll WRITE setBodyRoll)
    Q_PROPERTY(QString chatMessage READ getQStringChatMessage WRITE setChatMessage)

    Q_PROPERTY(glm::quat orientation READ getOrientation WRITE setOrientation)
    Q_PROPERTY(float headPitch READ getHeadPitch WRITE setHeadPitch)
    
    Q_PROPERTY(QUrl faceModelURL READ getFaceModelURL WRITE setFaceModelURL)
    Q_PROPERTY(QUrl skeletonModelURL READ getSkeletonModelURL WRITE setSkeletonModelURL)
public:
    AvatarData();
    ~AvatarData();

    const glm::vec3& getPosition() const { return _position; }
    void setPosition(const glm::vec3 position) { _position = position; }

    glm::vec3 getHandPosition() const;
    void setHandPosition(const glm::vec3& handPosition);

    QByteArray toByteArray();
    int parseData(const QByteArray& packet);

    //  Body Rotation
    float getBodyYaw() const { return _bodyYaw; }
    void setBodyYaw(float bodyYaw) { _bodyYaw = bodyYaw; }
    float getBodyPitch() const { return _bodyPitch; }
    void setBodyPitch(float bodyPitch) { _bodyPitch = bodyPitch; }
    float getBodyRoll() const { return _bodyRoll; }
    void setBodyRoll(float bodyRoll) { _bodyRoll = bodyRoll; }

    glm::quat getOrientation() const { return glm::quat(glm::radians(glm::vec3(_bodyPitch, _bodyYaw, _bodyRoll))); }
    void setOrientation(const glm::quat& orientation);

    // access to Head().set/getMousePitch
    float getHeadPitch() const { return _headData->getPitch(); }
    void setHeadPitch(float value) { _headData->setPitch(value); };

    //  Scale
    float getTargetScale() const { return _targetScale; }
    void setTargetScale(float targetScale) { _targetScale = targetScale; }
    void setClampedTargetScale(float targetScale);

    //  Hand State
    void setHandState(char s) { _handState = s; }
    char getHandState() const { return _handState; }

    // key state
    void setKeyState(KeyState s) { _keyState = s; }
    KeyState keyState() const { return _keyState; }

    // chat message
    void setChatMessage(const std::string& msg) { _chatMessage = msg; }
    void setChatMessage(const QString& string) { _chatMessage = string.toLocal8Bit().constData(); }
    const std::string& setChatMessage() const { return _chatMessage; }
    QString getQStringChatMessage() { return QString(_chatMessage.data()); }

    bool isChatCirclingEnabled() const { return _isChatCirclingEnabled; }
    const HeadData* getHeadData() const { return _headData; }
    const HandData* getHandData() const { return _handData; }

    void setHeadData(HeadData* headData) { _headData = headData; }
    void setHandData(HandData* handData) { _handData = handData; }

    virtual const glm::vec3& getVelocity() const { return vec3Zero; }

    virtual bool findParticleCollisions(const glm::vec3& particleCenter, float particleRadius, CollisionList& collisions) {
        return false;
    }

    bool hasIdentityChangedAfterParsing(const QByteArray& packet);
    QByteArray identityByteArray();
    
    const QUrl& getFaceModelURL() const { return _faceModelURL; }
    const QUrl& getSkeletonModelURL() const { return _skeletonModelURL; }
    const QString& getDisplayName() const { return _displayName; }
    virtual void setFaceModelURL(const QUrl& faceModelURL);
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);
    virtual void setDisplayName(const QString& displayName);
    
    virtual float getBoundingRadius() const { return 1.f; }

protected:
    glm::vec3 _position;
    glm::vec3 _handPosition;

    //  Body rotation
    float _bodyYaw;
    float _bodyPitch;
    float _bodyRoll;

    // Body scale
    float _targetScale;

    //  Hand state (are we grabbing something or not)
    char _handState;

    // key state
    KeyState _keyState;

    // chat message
    std::string _chatMessage;

    bool _isChatCirclingEnabled;

    HeadData* _headData;
    HandData* _handData;

    QUrl _faceModelURL;
    QUrl _skeletonModelURL;
    QString _displayName;

    int _displayNameWidth;
    bool _isShowDisplayName;
    float _displayNameAlpha;

private:
    // privatize the copy constructor and assignment operator so they cannot be called
    AvatarData(const AvatarData&);
    AvatarData& operator= (const AvatarData&);
};

#endif /* defined(__hifi__AvatarData__) */
