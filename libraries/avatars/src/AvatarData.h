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
#include <inttypes.h>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QtCore/QObject>
#include <QtCore/QUuid>
#include <QtCore/QVariantMap>

#include <RegisteredMetaTypes.h>

#include <NodeData.h>
#include "HeadData.h"
#include "HandData.h"

// First bitset
const int KEY_STATE_START_BIT = 0; // 1st and 2nd bits
const int HAND_STATE_START_BIT = 2; // 3rd and 4th bits
const int IS_FACESHIFT_CONNECTED = 4; // 5th bit
const int IS_CHAT_CIRCLING_ENABLED = 5;

const float MAX_AUDIO_LOUDNESS = 1000.0; // close enough for mouth animation

enum KeyState
{
    NO_KEY_DOWN = 0,
    INSERT_KEY_DOWN,
    DELETE_KEY_DOWN
};

class JointData;

class AvatarData : public NodeData {
    Q_OBJECT
    
    Q_PROPERTY(glm::vec3 position READ getPosition WRITE setPosition)
    Q_PROPERTY(glm::vec3 handPosition READ getHandPosition WRITE setHandPosition)
    Q_PROPERTY(float bodyYaw READ getBodyYaw WRITE setBodyYaw)
    Q_PROPERTY(float bodyPitch READ getBodyPitch WRITE setBodyPitch)
    Q_PROPERTY(float bodyRoll READ getBodyRoll WRITE setBodyRoll)
    Q_PROPERTY(QString chatMessage READ getQStringChatMessage WRITE setChatMessage)
public:
    AvatarData(Node* owningNode = NULL);
    ~AvatarData();
    
    const glm::vec3& getPosition() const { return _position; }
    void setPosition(const glm::vec3 position) { _position = position; }
    
    const glm::vec3& getHandPosition() const { return _handPosition; }
    void setHandPosition(const glm::vec3 handPosition) { _handPosition = handPosition; }
    
    int getBroadcastData(unsigned char* destinationBuffer);
    int parseData(unsigned char* sourceBuffer, int numBytes);
    
    QUuid& getUUID() { return _uuid; }
    void setUUID(const QUuid& uuid) { _uuid = uuid; }
    
    //  Body Rotation
    float getBodyYaw() const { return _bodyYaw; }
    void setBodyYaw(float bodyYaw) { _bodyYaw = bodyYaw; }
    float getBodyPitch() const { return _bodyPitch; }
    void setBodyPitch(float bodyPitch) { _bodyPitch = bodyPitch; }
    float getBodyRoll() const { return _bodyRoll; }
    void setBodyRoll(float bodyRoll) { _bodyRoll = bodyRoll; }
    
    //  Scale
    float getNewScale() const { return _newScale; }
    void setNewScale(float newScale) { _newScale = newScale; }
    
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

    const QUuid& getLeaderUUID() const { return _leaderUUID; }
    
    void setHeadData(HeadData* headData) { _headData = headData; }
    void setHandData(HandData* handData) { _handData = handData; }
    
protected:
    QUuid _uuid;
    
    glm::vec3 _position;
    glm::vec3 _handPosition;
    
    //  Body rotation
    float _bodyYaw;
    float _bodyPitch;
    float _bodyRoll;

    // Body scale
    float _newScale;

    // Following mode infos
    QUuid _leaderUUID;

    //  Hand state (are we grabbing something or not)
    char _handState;
    
    // key state
    KeyState _keyState;
    
    // chat message
    std::string _chatMessage;
    
    bool _isChatCirclingEnabled;
    
    std::vector<JointData> _joints;
    
    HeadData* _headData;
    HandData* _handData;
    
private:
    // privatize the copy constructor and assignment operator so they cannot be called
    AvatarData(const AvatarData&);
    AvatarData& operator= (const AvatarData&);
};

class JointData {
public:
    
    int jointID;
    glm::quat rotation;
};

#endif /* defined(__hifi__AvatarData__) */
