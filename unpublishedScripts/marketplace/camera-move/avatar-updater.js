/* eslint-env commonjs */
// ----------------------------------------------------------------------------

// helper module that performs the avatar movement update calculations

module.exports = AvatarUpdater;

var _utils = require('./modules/_utils.js'),
    assert = _utils.assert;

var movementUtils = require('./modules/movement-utils.js');

function AvatarUpdater(options) {
    options = options || {};
    assert(function assertion() {
        return typeof options.getCameraMovementSettings === 'function' &&
            typeof options.getMovementState === 'function' &&
            options.globalState;
    });

    var DEFAULT_MOTOR_TIMESCALE = 1e6; // a large value that matches Interface's default
    var EASED_MOTOR_TIMESCALE = 0.01;  // a small value to make Interface quickly apply MyAvatar.motorVelocity
    var EASED_MOTOR_THRESHOLD = 0.1;   // above this speed (m/s) EASED_MOTOR_TIMESCALE is used
    var ACCELERATION_MULTIPLIERS = { translation: 1, rotation: 1, zoom: 1 };
    var STAYGROUNDED_PITCH_THRESHOLD = 45.0; // degrees; ground level is maintained when pitch is within this threshold
    var MIN_DELTA_TIME = 0.0001; // to avoid math overflow, never consider dt less than this value
    var DEG_TO_RAD = Math.PI / 180.0;
    update.frameCount = 0;
    update.endTime = update.windowStartTime = _utils.getRuntimeSeconds();
    update.windowFrame = update.windowStartFrame= 0;

    this.update = update;
    this.options = options;
    this._resetMyAvatarMotor = _resetMyAvatarMotor;
    this._applyDirectPitchYaw = _applyDirectPitchYaw;

    var globalState = options.globalState;
    var getCameraMovementSettings = options.getCameraMovementSettings;
    var getMovementState = options.getMovementState;
    var _debugChannel = options.debugChannel;
    function update(dt) {
        update.frameCount++;
        var startTime = _utils.getRuntimeSeconds();
        var settings = getCameraMovementSettings(),
            EPSILON = settings.epsilon;

        var independentCamera = Camera.mode === 'independent',
            headPitch = MyAvatar.headPitch;

        var actualDeltaTime = startTime - update.endTime,
            practicalDeltaTime = Math.max(MIN_DELTA_TIME, actualDeltaTime),
            deltaTime;

        if (settings.useConstantDeltaTime) {
            deltaTime = settings.threadMode === movementUtils.CameraControls.ANIMATION_FRAME ?
                (1 / settings.fps) : (1 / 90);
        } else if (settings.threadMode === movementUtils.CameraControls.SCRIPT_UPDATE) {
            deltaTime = dt;
        } else {
            deltaTime = practicalDeltaTime;
        }

        var orientationProperty = settings.useHead ? 'headOrientation' : 'orientation',
            currentOrientation = independentCamera ? Camera.orientation : MyAvatar[orientationProperty],
            currentPosition = MyAvatar.position;

        var previousValues = globalState.previousValues,
            pendingChanges = globalState.pendingChanges,
            currentVelocities = globalState.currentVelocities;

        var movementState = getMovementState({ update: deltaTime }),
            targetState = movementUtils.applyEasing(deltaTime, 'easeIn', settings, movementState, ACCELERATION_MULTIPLIERS),
            dragState = movementUtils.applyEasing(deltaTime, 'easeOut', settings, currentVelocities, ACCELERATION_MULTIPLIERS);

        currentVelocities.integrate(targetState, currentVelocities, dragState, settings);

        var currentSpeed = Vec3.length(currentVelocities.translation),
            targetSpeed = Vec3.length(movementState.translation),
            verticalHold = movementState.isGrounded && settings.stayGrounded && Math.abs(headPitch) < STAYGROUNDED_PITCH_THRESHOLD;

        var deltaOrientation = Quat.fromVec3Degrees(Vec3.multiply(deltaTime, currentVelocities.rotation)),
            targetOrientation = Quat.normalize(Quat.multiply(currentOrientation, deltaOrientation));

        var targetVelocity = Vec3.multiplyQbyV(targetOrientation, currentVelocities.translation);

        if (verticalHold) {
            targetVelocity.y = 0;
        }

        var deltaPosition = Vec3.multiply(deltaTime, targetVelocity);

        _resetMyAvatarMotor(pendingChanges);

        if (!independentCamera) {
            var DriveModes = movementUtils.DriveModes;
            switch (settings.driveMode) {
                case DriveModes.MOTOR: {
                    if (currentSpeed > EPSILON || targetSpeed > EPSILON) {
                        var motorTimescale = (currentSpeed > EASED_MOTOR_THRESHOLD ? EASED_MOTOR_TIMESCALE : DEFAULT_MOTOR_TIMESCALE);
                        var motorPitch = Quat.fromPitchYawRollDegrees(headPitch, 180, 0),
                            motorVelocity = Vec3.multiplyQbyV(motorPitch, currentVelocities.translation);
                        if (verticalHold) {
                            motorVelocity.y = 0;
                        }
                        Object.assign(pendingChanges.MyAvatar, {
                            motorVelocity: motorVelocity,
                            motorTimescale: motorTimescale
                        });
                    }
                    break;
                }
                case DriveModes.THRUST: {
                    var thrustVector = currentVelocities.translation,
                        maxThrust = settings.translation.maxVelocity,
                        thrust;
                    if (targetSpeed > EPSILON) {
                        thrust = movementUtils.calculateThrust(maxThrust * 5, thrustVector, previousValues.thrust);
                    } else if (currentSpeed > 1 && Vec3.length(previousValues.thrust) > 1) {
                        thrust = Vec3.multiply(-currentSpeed / 10.0, thrustVector);
                    } else {
                        thrust = Vec3.ZERO;
                    }
                    if (thrust) {
                        thrust = Vec3.multiplyQbyV(MyAvatar[orientationProperty], thrust);
                        if (verticalHold) {
                            thrust.y = 0;
                        }
                    }
                    previousValues.thrust = pendingChanges.MyAvatar.setThrust = thrust;
                    break;
                }
                case DriveModes.POSITION: {
                    pendingChanges.MyAvatar.position = Vec3.sum(currentPosition, deltaPosition);
                    break;
                }
                default: {
                    throw new Error('unknown driveMode: ' + settings.driveMode);
                }
            }
        }

        var finalOrientation;
        switch (Camera.mode) {
            case 'mirror': // fall through
            case 'independent':
                targetOrientation = settings.preventRoll ? Quat.cancelOutRoll(targetOrientation) : targetOrientation;
                var boomVector = Vec3.multiply(-currentVelocities.zoom.z, Quat.getFront(targetOrientation)),
                    deltaCameraPosition = Vec3.sum(boomVector, deltaPosition);
                Object.assign(pendingChanges.Camera, {
                    position: Vec3.sum(Camera.position, deltaCameraPosition),
                    orientation: targetOrientation
                });
                break;
            case 'entity':
                finalOrientation = targetOrientation;
                break;
            default: // 'first person', 'third person'
                finalOrientation = targetOrientation;
                break;
        }

        if (settings.jitterTest) {
            finalOrientation = Quat.multiply(MyAvatar[orientationProperty], Quat.fromPitchYawRollDegrees(0, 60 * deltaTime, 0));
            // Quat.fromPitchYawRollDegrees(0, _utils.getRuntimeSeconds() * 60, 0)
        }

        if (finalOrientation) {
            if (settings.preventRoll) {
                finalOrientation = Quat.cancelOutRoll(finalOrientation);
            }
            previousValues.finalOrientation = pendingChanges.MyAvatar[orientationProperty] = Quat.normalize(finalOrientation);
        }

        if (!movementState.mouseSmooth && movementState.isRightMouseButton) {
            // directly apply mouse pitch and yaw when mouse smoothing is disabled
            _applyDirectPitchYaw(deltaTime, movementState, settings);
        }

        var endTime = _utils.getRuntimeSeconds();
        var cycleTime = endTime - update.endTime;
        update.endTime = endTime;

        pendingChanges.submit();

        if ((endTime - update.windowStartTime) > 3) {
            update.momentaryFPS = (update.frameCount - update.windowStartFrame) /
                (endTime - update.windowStartTime);
            update.windowStartFrame = update.frameCount;
            update.windowStartTime = endTime;
        }

        if (_debugChannel && update.windowStartFrame === update.frameCount) {
            Messages.sendLocalMessage(_debugChannel, JSON.stringify({
                threadFrames: update.threadFrames,
                frame: update.frameCount,
                threadMode: settings.threadMode,
                driveMode: settings.driveMode,
                orientationProperty: orientationProperty,
                isGrounded: movementState.isGrounded,
                targetAnimationFPS: settings.threadMode === movementUtils.CameraControls.ANIMATION_FRAME ? settings.fps : undefined,
                actualFPS: 1 / actualDeltaTime,
                effectiveAnimationFPS: 1 / deltaTime,
                seconds: {
                    startTime: startTime,
                    endTime: endTime
                },
                milliseconds: {
                    actualDeltaTime: actualDeltaTime * 1000,
                    deltaTime: deltaTime * 1000,
                    cycleTime: cycleTime * 1000,
                    calculationTime: (endTime - startTime) * 1000
                },
                finalOrientation: finalOrientation,
                thrust: thrust,
                maxVelocity: settings.translation,
                targetVelocity: targetVelocity,
                currentSpeed: currentSpeed,
                targetSpeed: targetSpeed
            }, 0, 2));
        }
    }

    function _resetMyAvatarMotor(targetObject) {
        if (MyAvatar.motorTimescale !== DEFAULT_MOTOR_TIMESCALE) {
            targetObject.MyAvatar.motorTimescale = DEFAULT_MOTOR_TIMESCALE;
        }
        if (MyAvatar.motorReferenceFrame !== 'avatar') {
            targetObject.MyAvatar.motorReferenceFrame = 'avatar';
        }
        if (Vec3.length(MyAvatar.motorVelocity)) {
            targetObject.MyAvatar.motorVelocity = Vec3.ZERO;
        }
    }

    function _applyDirectPitchYaw(deltaTime, movementState, settings) {
        var orientationProperty = settings.useHead ? 'headOrientation' : 'orientation',
            rotation = movementState.rotation,
            speed = Vec3.multiply(-DEG_TO_RAD / 2.0, settings.rotation.speed);

        var previousValues = globalState.previousValues,
            pendingChanges = globalState.pendingChanges,
            currentVelocities = globalState.currentVelocities;

        var previous = previousValues.pitchYawRoll,
            target = Vec3.multiply(deltaTime, Vec3.multiplyVbyV(rotation, speed)),
            pitchYawRoll = Vec3.mix(previous, target, 0.5),
            orientation = Quat.fromVec3Degrees(pitchYawRoll);

        previousValues.pitchYawRoll = pitchYawRoll;

        if (pendingChanges.MyAvatar.headOrientation || pendingChanges.MyAvatar.orientation) {
            var newOrientation = Quat.multiply(MyAvatar[orientationProperty], orientation);
            delete pendingChanges.MyAvatar.headOrientation;
            delete pendingChanges.MyAvatar.orientation;
            if (settings.preventRoll) {
                newOrientation = Quat.cancelOutRoll(newOrientation);
            }
            MyAvatar[orientationProperty] = newOrientation;
        } else if (pendingChanges.Camera.orientation) {
            var cameraOrientation = Quat.multiply(Camera.orientation, orientation);
            if (settings.preventRoll) {
                cameraOrientation = Quat.cancelOutRoll(cameraOrientation);
            }
            Camera.orientation = cameraOrientation;
        }
        currentVelocities.rotation = Vec3.ZERO;
    }
}
