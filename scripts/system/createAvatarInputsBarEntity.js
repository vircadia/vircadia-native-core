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
        var micBarDimensions = {x: 0.036, y: 0.048, z: 0.3};
        var bubbleIconDimensions = {x: 0.036, y: 0.036, z: 0.3};
        var micBarLocalPosition = {x: (-(micBarDimensions.x / 2)) - 0.2, y: -0.125, z: -0.5};
        var bubbleIconLocalPosition = {x: (micBarDimensions.x * 1.2 / 2) - 0.2, y: ((micBarDimensions.y - bubbleIconDimensions.y) / 2 - 0.125), z: -0.5};
        if (onCreateAvatarInputsBarEntity) {
            var props = {
                type: "Web",
                parentID: MyAvatar.SELF_ID,
                parentJointIndex: MyAvatar.getJointIndex("_CAMERA_MATRIX"),
                localPosition: micBarLocalPosition,
                localRotation: Quat.lookAtSimple(Camera.orientation, micBarLocalPosition),
                sourceUrl: Script.resourcesPath() + "qml/hifi/audio/MicBarApplication.qml",
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
                // y is 0.01 - (0.048 + 0.036) / 2
                // have 10% spacing separation between the entities
                localPosition: bubbleIconLocalPosition,
                localRotation: Quat.lookAtSimple(Camera.orientation, bubbleIconLocalPosition),
                sourceUrl: Script.resourcesPath() + "qml/BubbleIcon.qml",
                dimensions: bubbleIconDimensions,
                userData: {
                    grabbable: false
                },
            };
            bubbleIconEntity = Entities.addEntity(props, "local");
            console.log("creating entity");
        } else {
            console.log("deleting entity");
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
