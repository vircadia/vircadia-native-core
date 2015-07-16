//
//  MyAvatar.h
//  interface/src/avatar
//
//  Created by Mark Peng on 8/16/13.
//  Copyright 2012 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MyAvatar_h
#define hifi_MyAvatar_h

#include <SettingHandle.h>
#include <DynamicCharacterController.h>

#include "Avatar.h"

class ModelItemID;

enum eyeContactTarget {
    LEFT_EYE,
    RIGHT_EYE,
    MOUTH
};

class MyAvatar : public Avatar {
    Q_OBJECT
    Q_PROPERTY(bool shouldRenderLocally READ getShouldRenderLocally WRITE setShouldRenderLocally)
    Q_PROPERTY(glm::vec3 motorVelocity READ getScriptedMotorVelocity WRITE setScriptedMotorVelocity)
    Q_PROPERTY(float motorTimescale READ getScriptedMotorTimescale WRITE setScriptedMotorTimescale)
    Q_PROPERTY(QString motorReferenceFrame READ getScriptedMotorFrame WRITE setScriptedMotorFrame)
    Q_PROPERTY(QString collisionSoundURL READ getCollisionSoundURL WRITE setCollisionSoundURL)
    //TODO: make gravity feature work Q_PROPERTY(glm::vec3 gravity READ getGravity WRITE setGravity)

public:
	MyAvatar();
    ~MyAvatar();

    QByteArray toByteArray();
    void reset();
    void update(float deltaTime);
    void simulate(float deltaTime);
    void preRender(RenderArgs* renderArgs);
    void updateFromTrackers(float deltaTime);

    virtual void render(RenderArgs* renderArgs, const glm::vec3& cameraPosition, bool postLighting = false) override;
    virtual void renderBody(RenderArgs* renderArgs, ViewFrustum* renderFrustum, bool postLighting, float glowLevel = 0.0f) override;
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const override;

    // setters
    void setLeanScale(float scale) { _leanScale = scale; }
    void setShouldRenderLocally(bool shouldRender) { _shouldRender = shouldRender; }
    void setRealWorldFieldOfView(float realWorldFov) { _realWorldFieldOfView.set(realWorldFov); }

    // getters
    float getLeanScale() const { return _leanScale; }
    Q_INVOKABLE glm::vec3 getDefaultEyePosition() const;
    bool getShouldRenderLocally() const { return _shouldRender; }
    float getRealWorldFieldOfView() { return _realWorldFieldOfView.get(); }
    
    const QList<AnimationHandlePointer>& getAnimationHandles() const { return _animationHandles; }
    AnimationHandlePointer addAnimationHandle();
    void removeAnimationHandle(const AnimationHandlePointer& handle);
    
    /// Allows scripts to run animations.
    Q_INVOKABLE void startAnimation(const QString& url, float fps = 30.0f, float priority = 1.0f, bool loop = false,
        bool hold = false, float firstFrame = 0.0f, float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList());
    
    /// Stops an animation as identified by a URL.
    Q_INVOKABLE void stopAnimation(const QString& url);
    
    /// Starts an animation by its role, using the provided URL and parameters if the avatar doesn't have a custom
    /// animation for the role.
    Q_INVOKABLE void startAnimationByRole(const QString& role, const QString& url = QString(), float fps = 30.0f,
        float priority = 1.0f, bool loop = false, bool hold = false, float firstFrame = 0.0f,
        float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList());
    
    /// Stops an animation identified by its role.
    Q_INVOKABLE void stopAnimationByRole(const QString& role);

    Q_INVOKABLE AnimationDetails getAnimationDetailsByRole(const QString& role);
    Q_INVOKABLE AnimationDetails getAnimationDetails(const QString& url);
    
    // get/set avatar data
    void saveData();
    void loadData();

    void saveAttachmentData(const AttachmentData& attachment) const;
    AttachmentData loadAttachmentData(const QUrl& modelURL, const QString& jointName = QString()) const;

    //  Set what driving keys are being pressed to control thrust levels
    void clearDriveKeys();
    void setDriveKeys(int key, float val) { _driveKeys[key] = val; };
    bool getDriveKeys(int key) { return _driveKeys[key] != 0.0f; };
    
    void relayDriveKeysToCharacterController();

    bool isMyAvatar() const { return true; }
    
    eyeContactTarget getEyeContactTarget();

    virtual int parseDataFromBuffer(const QByteArray& buffer);
    
    static void sendKillAvatar();
    
    Q_INVOKABLE glm::vec3 getTrackedHeadPosition() const { return _trackedHeadPosition; }
    Q_INVOKABLE glm::vec3 getHeadPosition() const { return getHead()->getPosition(); }
    Q_INVOKABLE float getHeadFinalYaw() const { return getHead()->getFinalYaw(); }
    Q_INVOKABLE float getHeadFinalRoll() const { return getHead()->getFinalRoll(); }
    Q_INVOKABLE float getHeadFinalPitch() const { return getHead()->getFinalPitch(); }
    Q_INVOKABLE float getHeadDeltaPitch() const { return getHead()->getDeltaPitch(); }
    
    Q_INVOKABLE glm::vec3 getEyePosition() const { return getHead()->getEyePosition(); }
    
    Q_INVOKABLE glm::vec3 getTargetAvatarPosition() const { return _targetAvatarPosition; }
    AvatarWeakPointer getLookAtTargetAvatar() const { return _lookAtTargetAvatar; }
    void updateLookAtTargetAvatar();
    void clearLookAtTargetAvatar();
    
    virtual void setJointRotations(QVector<glm::quat> jointRotations);
    virtual void setJointData(int index, const glm::quat& rotation);
    virtual void clearJointData(int index);
    virtual void clearJointsData();

    Q_INVOKABLE void useFullAvatarURL(const QUrl& fullAvatarURL, const QString& modelName = QString());
    Q_INVOKABLE void useHeadURL(const QUrl& headURL, const QString& modelName = QString());
    Q_INVOKABLE void useBodyURL(const QUrl& bodyURL, const QString& modelName = QString());
    Q_INVOKABLE void useHeadAndBodyURLs(const QUrl& headURL, const QUrl& bodyURL, const QString& headName = QString(), const QString& bodyName = QString());

    Q_INVOKABLE bool getUseFullAvatar() const { return _useFullAvatar; }
    Q_INVOKABLE const QUrl& getFullAvatarURLFromPreferences() const { return _fullAvatarURLFromPreferences; }
    Q_INVOKABLE const QUrl& getHeadURLFromPreferences() const { return _headURLFromPreferences; }
    Q_INVOKABLE const QUrl& getBodyURLFromPreferences() const { return _skeletonURLFromPreferences; }

    Q_INVOKABLE const QString& getHeadModelName() const { return _headModelName; }
    Q_INVOKABLE const QString& getBodyModelName() const { return _bodyModelName; }
    Q_INVOKABLE const QString& getFullAvartarModelName() const { return _fullAvatarModelName; }

    Q_INVOKABLE QString getModelDescription() const;

    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData);

    virtual glm::vec3 getSkeletonPosition() const;
    void updateLocalAABox();
    DynamicCharacterController* getCharacterController() { return &_characterController; }
    
    void clearJointAnimationPriorities();

    glm::vec3 getScriptedMotorVelocity() const { return _scriptedMotorVelocity; }
    float getScriptedMotorTimescale() const { return _scriptedMotorTimescale; }
    QString getScriptedMotorFrame() const;

    void setScriptedMotorVelocity(const glm::vec3& velocity);
    void setScriptedMotorTimescale(float timescale);
    void setScriptedMotorFrame(QString frame);

    const QString& getCollisionSoundURL() {return _collisionSoundURL; }
    void setCollisionSoundURL(const QString& url);

    void clearScriptableSettings();

    virtual void attach(const QString& modelURL, const QString& jointName = QString(),
        const glm::vec3& translation = glm::vec3(), const glm::quat& rotation = glm::quat(), float scale = 1.0f,
        bool allowDuplicates = false, bool useSaved = true);
        
    /// Renders a laser pointer for UI picking
    void renderLaserPointers(gpu::Batch& batch);
    glm::vec3 getLaserPointerTipPosition(const PalmData* palm);
    
    const RecorderPointer getRecorder() const { return _recorder; }
    const PlayerPointer getPlayer() const { return _player; }
    
    float getBoomLength() const { return _boomLength; }
    void setBoomLength(float boomLength) { _boomLength = boomLength; }
    
    static const float ZOOM_MIN;
    static const float ZOOM_MAX;
    static const float ZOOM_DEFAULT;
    
public slots:
    void increaseSize();
    void decreaseSize();
    void resetSize();
    
    void goToLocation(const glm::vec3& newPosition,
                      bool hasOrientation = false, const glm::quat& newOrientation = glm::quat(),
                      bool shouldFaceLocation = false);

    //  Set/Get update the thrust that will move the avatar around
    void addThrust(glm::vec3 newThrust) { _thrust += newThrust; };
    glm::vec3 getThrust() { return _thrust; };
    void setThrust(glm::vec3 newThrust) { _thrust = newThrust; }

    void updateMotionBehavior();

    glm::vec3 getLeftPalmPosition();
    glm::quat getLeftPalmRotation();
    glm::vec3 getRightPalmPosition();
    glm::quat getRightPalmRotation();

    void clearReferential();
    bool setModelReferential(const QUuid& id);
    bool setJointReferential(const QUuid& id, int jointIndex);
    
    bool isRecording();
    qint64 recorderElapsed();
    void startRecording();
    void stopRecording();
    void saveRecording(QString filename);
    void loadLastRecording();

    virtual void rebuildSkeletonBody();
    
signals:
    void transformChanged();
    void newCollisionSoundURL(const QUrl& url);
    void collisionWithEntity(const Collision& collision);

private:

    bool cameraInsideHead() const;

    // These are made private for MyAvatar so that you will use the "use" methods instead
    virtual void setFaceModelURL(const QUrl& faceModelURL);
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);

    void setVisibleInSceneIfReady(Model* model, render::ScenePointer scene, bool visiblity);

    glm::vec3 _gravity;

    float _driveKeys[MAX_DRIVE_KEYS];
    bool _wasPushing;
    bool _isPushing;
    bool _isBraking;
    
    float _boomLength;

    float _trapDuration; // seconds that avatar has been trapped by collisions
    glm::vec3 _thrust;  // impulse accumulator for outside sources

    glm::vec3 _keyboardMotorVelocity; // target local-frame velocity of avatar (keyboard)
    float _keyboardMotorTimescale; // timescale for avatar to achieve its target velocity
    glm::vec3 _scriptedMotorVelocity; // target local-frame velocity of avatar (script)
    float _scriptedMotorTimescale; // timescale for avatar to achieve its target velocity
    int _scriptedMotorFrame;
    quint32 _motionBehaviors;
    QString _collisionSoundURL;

    DynamicCharacterController _characterController;

    AvatarWeakPointer _lookAtTargetAvatar;
    glm::vec3 _targetAvatarPosition;
    bool _shouldRender;
    bool _billboardValid;
    float _oculusYawOffset;

    QList<AnimationHandlePointer> _animationHandles;
    
    bool _feetTouchFloor;
    eyeContactTarget _eyeContactTarget;

    RecorderPointer _recorder;
    
    glm::vec3 _trackedHeadPosition;
    
    Setting::Handle<float> _realWorldFieldOfView;
    
	// private methods
    void updateOrientation(float deltaTime);
    glm::vec3 applyKeyboardMotor(float deltaTime, const glm::vec3& velocity, bool isHovering);
    glm::vec3 applyScriptedMotor(float deltaTime, const glm::vec3& velocity);
    void updatePosition(float deltaTime);
    void updateCollisionSound(const glm::vec3& penetration, float deltaTime, float frequency);
    void maybeUpdateBillboard();
    
    // Avatar Preferences
    bool _useFullAvatar = false;
    QUrl _fullAvatarURLFromPreferences;
    QUrl _headURLFromPreferences;
    QUrl _skeletonURLFromPreferences;
    
    QString _headModelName;
    QString _bodyModelName;
    QString _fullAvatarModelName;

    // used for rendering when in first person view or when in an HMD.
    SkeletonModel _firstPersonSkeletonModel;
    bool _prevShouldDrawHead;
};

#endif // hifi_MyAvatar_h
