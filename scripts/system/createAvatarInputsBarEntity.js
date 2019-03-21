"use strict";

(function(){
    var AppUi = Script.require("appUi");

    var ui;

    var onCreateAvatarInputsBarEntity = false;
    var micBarEntity = null;
    var bubbleIconEntity = null;
    var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
    var AVATAR_INPUTS_EDIT_QML_SOURCE = "hifi/EditAvatarInputsBar.qml";

    // DPI
    var ENTITY_DPI = 60.0;
    // QML NATURAL DIMENSIONS
    var MIC_BAR_DIMENSIONS = Vec3.multiply(30.0 / ENTITY_DPI, {x: 0.036, y: 0.048, z: 0.3});
    var BUBBLE_ICON_DIMENSIONS = Vec3.multiply(30.0 / ENTITY_DPI, {x: 0.036, y: 0.036, z: 0.3});
    // ENTITY NAMES
    var MIC_BAR_NAME = "AvatarInputsMicBarEntity";
    var BUBBLE_ICON_NAME = "AvatarInputsBubbleIconEntity";
    // CONSTANTS
    var LOCAL_POSITION_X_OFFSET = -0.2;
    var LOCAL_POSITION_Y_OFFSET = -0.125;
    var LOCAL_POSITION_Z_OFFSET = -0.5;

    function fromQml(message) {
        if (message.method === "reposition") {
            var micBarLocalPosition = Entities.getEntityProperties(micBarEntity).localPosition;
            var bubbleIconLocalPosition = Entities.getEntityProperties(bubbleIconEntity).localPosition;
            var newMicBarLocalPosition, newBubbleIconLocalPosition;
            if (message.x !== undefined) {
                newMicBarLocalPosition = { x: -((MIC_BAR_DIMENSIONS.x) / 2) + message.x, y: micBarLocalPosition.y, z: micBarLocalPosition.z };
                newBubbleIconLocalPosition = { x: ((MIC_BAR_DIMENSIONS.x) * 1.2 / 2) + message.x, y: bubbleIconLocalPosition.y, z: bubbleIconLocalPosition.z };
            } else if (message.y !== undefined) {
                newMicBarLocalPosition = { x: micBarLocalPosition.x, y: message.y, z: micBarLocalPosition.z };
                newBubbleIconLocalPosition = { x: bubbleIconLocalPosition.x, y: ((MIC_BAR_DIMENSIONS.y - BUBBLE_ICON_DIMENSIONS.y) / 2 + message.y), z: bubbleIconLocalPosition.z };
            } else if (message.z !== undefined) {
                newMicBarLocalPosition = { x: micBarLocalPosition.x, y: micBarLocalPosition.y, z: message.z };
                newBubbleIconLocalPosition = { x: bubbleIconLocalPosition.x, y: bubbleIconLocalPosition.y, z: message.z };
            }
            var micBarProps = {
                localPosition: newMicBarLocalPosition
            };
            var bubbleIconProps = {
                localPosition: newBubbleIconLocalPosition
            };

            Entities.editEntity(micBarEntity, micBarProps);
            Entities.editEntity(bubbleIconEntity, bubbleIconProps);
        } else if (message.method === "setVisible") {
            if (message.visible !== undefined) {
                var props = {
                    visible: message.visible
                };
                Entities.editEntity(micBarEntity, props);
                Entities.editEntity(bubbleIconEntity, props);
            }
        } else if (message.method === "print") {
            // prints the local position into the hifi log.
            var micBarLocalPosition = Entities.getEntityProperties(micBarEntity).localPosition;
            var bubbleIconLocalPosition = Entities.getEntityProperties(bubbleIconEntity).localPosition;
            console.log("mic bar local position is at " + JSON.stringify(micBarLocalPosition));
            console.log("bubble icon local position is at " + JSON.stringify(bubbleIconLocalPosition));
        }
    };

    function createEntities() {
        if (micBarEntity != null && bubbleIconEntity != null) {
            return;
        }
        // POSITIONS
        var micBarLocalPosition = {x: (-(MIC_BAR_DIMENSIONS.x / 2)) + LOCAL_POSITION_X_OFFSET, y: LOCAL_POSITION_Y_OFFSET, z: LOCAL_POSITION_Z_OFFSET};
        var bubbleIconLocalPosition = {x: (MIC_BAR_DIMENSIONS.x * 1.2 / 2) + LOCAL_POSITION_X_OFFSET, y: ((MIC_BAR_DIMENSIONS.y - BUBBLE_ICON_DIMENSIONS.y) / 2 + LOCAL_POSITION_Y_OFFSET), z: LOCAL_POSITION_Z_OFFSET};
        var props = {
            type: "Web",
            name: MIC_BAR_NAME,
            parentID: MyAvatar.SELF_ID,
            parentJointIndex: MyAvatar.getJointIndex("_CAMERA_MATRIX"),
            localPosition: micBarLocalPosition,
            localRotation: Quat.cancelOutRollAndPitch(Quat.lookAtSimple(Camera.orientation, micBarLocalPosition)),
            sourceUrl: Script.resourcesPath() + "qml/hifi/audio/MicBarApplication.qml",
            // cutoff alpha for detecting transparency
            alpha: 0.98,
            dimensions: MIC_BAR_DIMENSIONS,
            dpi: ENTITY_DPI,
            drawInFront: true,
            userData: {
                grabbable: false
            },
        };
        micBarEntity = Entities.addEntity(props, "local");
        var props = {
            type: "Web",
            name: BUBBLE_ICON_NAME,
            parentID: MyAvatar.SELF_ID,
            parentJointIndex: MyAvatar.getJointIndex("_CAMERA_MATRIX"),
            localPosition: bubbleIconLocalPosition,
            localRotation: Quat.cancelOutRollAndPitch(Quat.lookAtSimple(Camera.orientation, bubbleIconLocalPosition)),
            sourceUrl: Script.resourcesPath() + "qml/BubbleIcon.qml",
            // cutoff alpha for detecting transparency
            alpha: 0.98,
            dimensions: BUBBLE_ICON_DIMENSIONS,
            dpi: ENTITY_DPI,
            drawInFront: true,
            userData: {
                grabbable: false
            },
        };
        bubbleIconEntity = Entities.addEntity(props, "local");
        tablet.loadQMLSource(AVATAR_INPUTS_EDIT_QML_SOURCE);
    };
    function cleanup() {
        if (micBarEntity) {
            Entities.deleteEntity(micBarEntity);
        }
        if (bubbleIconEntity) {
            Entities.deleteEntity(bubbleIconEntity);
        }
    };

    function setup() {
        ui = new AppUi({
            buttonName: "AVBAR",
            home: Script.resourcesPath() + "qml/hifi/EditAvatarInputsBar.qml",
            onMessage: fromQml,
            onOpened: createEntities,
            // onClosed: cleanup,
            normalButton: "icons/tablet-icons/edit-i.svg",
            activeButton: "icons/tablet-icons/edit-a.svg",
        });
    };

    setup();

    Script.scriptEnding.connect(cleanup);

}());
