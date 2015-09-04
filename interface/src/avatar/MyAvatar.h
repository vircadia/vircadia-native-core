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
#include <Rig.h>

#include "Avatar.h"

class ModelItemID;
class AnimNode;

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
    MyAvatar(RigPointer rig);
    ~MyAvatar();


    void reset();
    void update(float deltaTime);
    void preRender(RenderArgs* renderArgs);

    const glm::mat4& getHMDSensorMatrix() const { return _hmdSensorMatrix; }
    const glm::vec3& getHMDSensorPosition() const { return _hmdSensorPosition; }
    const glm::quat& getHMDSensorOrientation() const { return _hmdSensorOrientation; }
    glm::mat4 getSensorToWorldMatrix() const;

    // best called at start of main loop just after we have a fresh hmd pose.
    // update internal body position from new hmd pose.
    void updateFromHMDSensorMatrix(const glm::mat4& hmdSensorMatrix);

    // best called at end of main loop, just before rendering.
    // update sensor to world matrix from current body position and hmd sensor.
    // This is so the correct camera can be used for rendering.
    void updateSensorToWorldMatrix();

    void setLeanScale(float scale) { _leanScale = scale; }
    void setRealWorldFieldOfView(float realWorldFov) { _realWorldFieldOfView.set(realWorldFov); }

    float getLeanScale() const { return _leanScale; }
    Q_INVOKABLE glm::vec3 getDefaultEyePosition() const;

    float getRealWorldFieldOfView() { return _realWorldFieldOfView.get(); }

    const QList<AnimationHandlePointer>& getAnimationHandles() const { return _rig->getAnimationHandles(); }
    AnimationHandlePointer addAnimationHandle() { return _rig->createAnimationHandle(); }
    void removeAnimationHandle(const AnimationHandlePointer& handle) { _rig->removeAnimationHandle(handle); }
    /// Allows scripts to run animations.
    Q_INVOKABLE void startAnimation(const QString& url, float fps = 30.0f, float priority = 1.0f, bool loop = false,
                                    bool hold = false, float firstFrame = 0.0f,
                                    float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList());

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
    void clearJointAnimationPriorities();

    // get/set avatar data
    void saveData();
    void loadData();

    void saveAttachmentData(const AttachmentData& attachment) const;
    AttachmentData loadAttachmentData(const QUrl& modelURL, const QString& jointName = QString()) const;

    //  Set what driving keys are being pressed to control thrust levels
    void clearDriveKeys();
    void setDriveKeys(int key, float val) { _driveKeys[key] = val; };
    void relayDriveKeysToCharacterController();

    eyeContactTarget getEyeContactTarget();

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
    Q_INVOKABLE const QUrl& getFullAvatarURLFromPreferences() const { return _fullAvatarURLFromPreferences; }
    Q_INVOKABLE const QString& getFullAvatarModelName() const { return _fullAvatarModelName; }

    virtual void setAttachmentData(const QVector<AttachmentData>& attachmentData);

    DynamicCharacterController* getCharacterController() { return &_characterController; }


    const QString& getCollisionSoundURL() {return _collisionSoundURL; }
    void setCollisionSoundURL(const QString& url);

    void clearScriptableSettings();

    /// Renders a laser pointer for UI picking

    glm::vec3 getLaserPointerTipPosition(const PalmData* palm);

    float getBoomLength() const { return _boomLength; }
    void setBoomLength(float boomLength) { _boomLength = boomLength; }

    static const float ZOOM_MIN;
    static const float ZOOM_MAX;
    static const float ZOOM_DEFAULT;

    bool getStandingHMDSensorMode() const { return _standingHMDSensorMode; }

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

    void updateMotionBehaviorFromMenu();
    void updateStandingHMDModeFromMenu();

    glm::vec3 getLeftPalmPosition();
    glm::vec3 getLeftPalmVelocity();
    glm::vec3 getLeftPalmAngularVelocity();
    glm::quat getLeftPalmRotation();
    glm::vec3 getRightPalmPosition();
    glm::vec3 getRightPalmVelocity();
    glm::vec3 getRightPalmAngularVelocity();
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

    void setEnableRigAnimations(bool isEnabled);
    void setEnableAnimGraph(bool isEnabled);
    void setEnableDebugDrawBindPose(bool isEnabled);
    void setEnableDebugDrawAnimPose(bool isEnabled);
    void setEnableMeshVisible(bool isEnabled);

signals:
    void transformChanged();
    void newCollisionSoundURL(const QUrl& url);
    void collisionWithEntity(const Collision& collision);

private:

    glm::vec3 getWorldBodyPosition() const;
    glm::quat getWorldBodyOrientation() const;
    QByteArray toByteArray(bool cullSmallChanges, bool sendAll);
    void simulate(float deltaTime);
    void updateFromTrackers(float deltaTime);
    virtual void render(RenderArgs* renderArgs, const glm::vec3& cameraPositio) override;
    virtual void renderBody(RenderArgs* renderArgs, ViewFrustum* renderFrustum, float glowLevel = 0.0f) override;
    virtual bool shouldRenderHead(const RenderArgs* renderArgs) const override;
    void setShouldRenderLocally(bool shouldRender) { _shouldRender = shouldRender; }
    bool getShouldRenderLocally() const { return _shouldRender; }
    bool getDriveKeys(int key) { return _driveKeys[key] != 0.0f; };
    bool isMyAvatar() const { return true; }
    virtual int parseDataFromBuffer(const QByteArray& buffer);
    virtual glm::vec3 getSkeletonPosition() const;

    glm::vec3 getScriptedMotorVelocity() const { return _scriptedMotorVelocity; }
    float getScriptedMotorTimescale() const { return _scriptedMotorTimescale; }
    QString getScriptedMotorFrame() const;
    void setScriptedMotorVelocity(const glm::vec3& velocity);
    void setScriptedMotorTimescale(float timescale);
    void setScriptedMotorFrame(QString frame);
    virtual void attach(const QString& modelURL, const QString& jointName = QString(),
                        const glm::vec3& translation = glm::vec3(), const glm::quat& rotation = glm::quat(), float scale = 1.0f,
                        bool allowDuplicates = false, bool useSaved = true);

    void renderLaserPointers(gpu::Batch& batch);
    const RecorderPointer getRecorder() const { return _recorder; }
    const PlayerPointer getPlayer() const { return _player; }

    bool cameraInsideHead() const;

    // These are made private for MyAvatar so that you will use the "use" methods instead
    virtual void setFaceModelURL(const QUrl& faceModelURL);
    virtual void setSkeletonModelURL(const QUrl& skeletonModelURL);

    void setVisibleInSceneIfReady(Model* model, render::ScenePointer scene, bool visiblity);

    // derive avatar body position and orientation from the current HMD Sensor location.
    // results are in sensor space
    glm::mat4 deriveBodyFromHMDSensor() const;

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
    void initHeadBones();
    void initAnimGraph();
    void destroyAnimGraph();

    // Avatar Preferences
    QUrl _fullAvatarURLFromPreferences;
    QString _fullAvatarModelName;

    // cache of the current HMD sensor position and orientation
    // in sensor space.
    glm::mat4 _hmdSensorMatrix;
    glm::quat _hmdSensorOrientation;
    glm::vec3 _hmdSensorPosition;

    // cache of the current body position and orientation of the avatar's body,
    // in sensor space.
    glm::mat4 _bodySensorMatrix;

    // used to transform any sensor into world space, including the _hmdSensorMat, or hand controllers.
    glm::mat4 _sensorToWorldMatrix;

    bool _standingHMDSensorMode;

    bool _goToPending;
    glm::vec3 _goToPosition;
    glm::quat _goToOrientation;

    std::unordered_set<int> _headBoneSet;
    RigPointer _rig;
    bool _prevShouldDrawHead;

    bool _enableDebugDrawBindPose = false;
    bool _enableDebugDrawAnimPose = false;
};

#endif // hifi_MyAvatar_h
