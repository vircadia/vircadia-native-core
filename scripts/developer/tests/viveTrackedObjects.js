
var TRACKED_OBJECT_POSES = [
    "TrackedObject00", "TrackedObject01", "TrackedObject02", "TrackedObject03",
    "TrackedObject04", "TrackedObject05", "TrackedObject06", "TrackedObject07",
    "TrackedObject08", "TrackedObject09", "TrackedObject10", "TrackedObject11",
    "TrackedObject12", "TrackedObject13", "TrackedObject14", "TrackedObject15"
];

function init() {
    Script.update.connect(update);
}

function shutdown() {
    Script.update.disconnect(update);

    TRACKED_OBJECT_POSES.forEach(function (key) {
        DebugDraw.removeMyAvatarMarker(key);
    });
}

var WHITE = {x: 1, y: 1, z: 1, w: 1};

function update(dt) {
    if (Controller.Hardware.Vive) {
        TRACKED_OBJECT_POSES.forEach(function (key) {
            var pose = Controller.getPoseValue(Controller.Hardware.Vive[key]);
            if (pose.valid) {
                DebugDraw.addMyAvatarMarker(key, pose.rotation, pose.translation, WHITE);
            } else {
                DebugDraw.removeMyAvatarMarker(key);
            }
        });
    }
}

init();
