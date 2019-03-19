"use strict";

(function(){

    var button;
    var buttonName = "AVBAR";
    var onCreateAvatarInputsBarEntity = false;
    var micBarEntity, bubbleIconEntity;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");

    function onClicked(){
        onCreateAvatarInputsBarEntity = !onCreateAvatarInputsBarEntity;
        button.editProperties({isActive: onCreateAvatarInputsBarEntity});
        // QML NATURAL DIMENSIONS
        var MIC_BAR_DIMENSIONS = {x: 0.036, y: 0.048, z: 0.3};
        var BUBBLE_ICON_DIMENSIONS = {x: 0.036, y: 0.036, z: 0.3};
        // CONSTANTS
        var LOCAL_POSITION_X_OFFSET = -0.2;
        var LOCAL_POSITION_Y_OFFSET = -0.125;
        var LOCAL_POSITION_Z_OFFSET = -0.5;
        // POSITIONS
        var micBarLocalPosition = {x: (-(micBarDimensions.x / 2)) + LOCAL_POSITION_X_OFFSET, y: LOCAL_POSITION_Y_OFFSET, z: LOCAL_POSITION_Z_OFFSET};
        var bubbleIconLocalPosition = {x: (micBarDimensions.x * 1.2 / 2) + LOCAL_POSITION_X_OFFSET, y: ((micBarDimensions.y - bubbleIconDimensions.y) / 2 + LOCAL_POSITION_Y_OFFSET), z: LOCAL_POSITION_Z_OFFSET};
        if (onCreateAvatarInputsBarEntity) {
            var props = {
                type: "Web",
                parentID: MyAvatar.SELF_ID,
                parentJointIndex: MyAvatar.getJointIndex("_CAMERA_MATRIX"),
                localPosition: micBarLocalPosition,
                localRotation: Quat.cancelOutRollAndPitch(Quat.lookAtSimple(Camera.orientation, micBarLocalPosition)),
                sourceUrl: Script.resourcesPath() + "qml/hifi/audio/MicBarApplication.qml",
                // cutoff alpha for detecting transparency
                alpha: 0.98,
                dimensions: micBarDimensions,
                userData: {
                    grabbable: false
                },
            };
            micBarEntity = Entities.addEntity(props, "local");
            var props = {
                type: "Web",
                parentID: MyAvatar.SELF_ID,
                parentJointIndex: MyAvatar.getJointIndex("_CAMERA_MATRIX"),
                localPosition: bubbleIconLocalPosition,
                localRotation: Quat.cancelOutRollAndPitch(Quat.lookAtSimple(Camera.orientation, bubbleIconLocalPosition)),
                sourceUrl: Script.resourcesPath() + "qml/BubbleIcon.qml",
                // cutoff alpha for detecting transparency
                alpha: 0.98,
                dimensions: bubbleIconDimensions,
                userData: {
                    grabbable: false
                },
            };
            bubbleIconEntity = Entities.addEntity(props, "local");
        } else {
            Entities.deleteEntity(micBarEntity);
            Entities.deleteEntity(bubbleIconEntity);
        }
    };

    function setup() {
        button = tablet.addButton({
        icon: "icons/tablet-icons/edit-i.svg",
        activeIcon: "icons/tablet-icons/edit-a.svg",
        text: buttonName
        });
        button.clicked.connect(onClicked);
    };

    setup();

    Script.scriptEnding.connect(function() {
        if (micBarEntity) {
            Entities.deleteEntity(micBarEntity);
        }
        if (bubbleIconEntity) {
            Entities.deleteEntity(bubbleIconEntity);
        }
        tablet.removeButton(button);
    });

}());
