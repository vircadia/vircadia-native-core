
var skeletonModelURL = "";
var jointCount = 0;

var excludedRoles = ["rightHandGraspOpen", "rightHandGraspClosed", "leftHandGraspOpen", "leftHandGraspClosed"];
var IDLE_URL = "http://hifi-content.s3.amazonaws.com/ozan/dev/anim/standard_anims_160127/idle.fbx";

function overrideAnims() {
    var roles = MyAvatar.getAnimationRoles();
    var i, l = roles.length;
    for (i = 0; i < l; i++) {
        if (excludedRoles.indexOf(roles[i]) == -1) {
            MyAvatar.overrideRoleAnimation(roles[i], IDLE_URL, 30, false, 1, 1);
        }
    }
}

function restoreAnims() {
    var roles = MyAvatar.getAnimationRoles();
    var i, l = roles.length;
    for (i = 0; i < l; i++) {
        if (excludedRoles.indexOf(roles[i]) == -1) {
            MyAvatar.restoreRoleAnimation(roles[i]);
        }
    }
}

overrideAnims();

MyAvatar.onLoadComplete.connect(function () {
    overrideAnims();
});

Script.scriptEnding.connect(function () {
    restoreAnims();
});


