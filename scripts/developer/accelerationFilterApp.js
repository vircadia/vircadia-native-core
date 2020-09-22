var LEFT_HAND_INDEX = 0;
var RIGHT_HAND_INDEX = 1;
var LEFT_FOOT_INDEX = 2;
var RIGHT_FOOT_INDEX = 3;
var HIPS_INDEX = 4;
var SPINE2_INDEX = 5;

var mappingJson = {
    name: "com.highfidelity.testing.accelerationTest",
    channels: [
        {
            from: "Standard.LeftHand",
            to: "Actions.LeftHand",
            filters: [
                {
                    type: "accelerationLimiter",
                    rotationAccelerationLimit: 2000.0,
                    translationAccelerationLimit: 100.0,
                }
            ]
        },
        {
            from: "Standard.RightHand",
            to: "Actions.RightHand",
            filters: [
                {
                    type: "accelerationLimiter",
                    rotationAccelerationLimit: 2000.0,
                    translationAccelerationLimit: 100.0,
                }
            ]
        },
        {
            from: "Standard.LeftFoot",
            to: "Actions.LeftFoot",
            filters: [
                {
                    type: "accelerationLimiter",
                    rotationAccelerationLimit: 2000.0,
                    translationAccelerationLimit: 100.0,
                }
            ]
        },
        {
            from: "Standard.RightFoot",
            to: "Actions.RightFoot",
            filters: [
                {
                    type: "accelerationLimiter",
                    rotationAccelerationLimit: 2000.0,
                    translationAccelerationLimit: 100.0,
                }
            ]
        },
        {
            from: "Standard.Hips",
            to: "Actions.Hips",
            filters: [
                {
                    type: "accelerationLimiter",
                    rotationAccelerationLimit: 2000.0,
                    translationAccelerationLimit: 100.0,
                }
            ]
        },
        {
            from: "Standard.Spine2",
            to: "Actions.Spine2",
            filters: [
                {
                    type: "accelerationLimiter",
                    rotationAccelerationLimit: 2000.0,
                    translationAccelerationLimit: 100.0,
                }
            ]
        }
    ]
};

//
// tablet app boiler plate
//

var TABLET_BUTTON_NAME = "ACCFILT";
var HTML_URL = Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/html/accelerationFilterApp.html?2");

var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
var tabletButton = tablet.addButton({
    text: TABLET_BUTTON_NAME,
    icon: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/tpose-i.svg"),
    activeIcon: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/icons/tpose-a.svg")
});

tabletButton.clicked.connect(function () {
    if (shown) {
        tablet.gotoHomeScreen();
    } else {
        tablet.gotoWebScreen(HTML_URL);
    }
});

var shown = false;

function onScreenChanged(type, url) {
    if (type === "Web" && url === HTML_URL) {
        tabletButton.editProperties({isActive: true});
        if (!shown) {
            // hook up to event bridge
            tablet.webEventReceived.connect(onWebEventReceived);
            shownChanged(true);
        }
        shown = true;
    } else {
        tabletButton.editProperties({isActive: false});
        if (shown) {
            // disconnect from event bridge
            tablet.webEventReceived.disconnect(onWebEventReceived);
            shownChanged(false);
        }
        shown = false;
    }
}

function getTranslationAccelerationLimit(i) {
    return mappingJson.channels[i].filters[0].translationAccelerationLimit;
}
function setTranslationAccelerationLimit(i, value) {
    mappingJson.channels[i].filters[0].translationAccelerationLimit = value;
    mappingChanged();
}
function getRotationAccelerationLimit(i) {
    return mappingJson.channels[i].filters[0].rotationAccelerationLimit;
}
function setRotationAccelerationLimit(i, value) {
    mappingJson.channels[i].filters[0].rotationAccelerationLimit = value; mappingChanged();
}

function onWebEventReceived(msg) {
    if (msg.name === "init-complete") {
        var values = [
            {name: "left-hand-translation-acceleration-limit", val: getTranslationAccelerationLimit(LEFT_HAND_INDEX), checked: false},
            {name: "left-hand-rotation-acceleration-limit", val: getRotationAccelerationLimit(LEFT_HAND_INDEX), checked: false},
            {name: "right-hand-translation-acceleration-limit", val: getTranslationAccelerationLimit(RIGHT_HAND_INDEX), checked: false},
            {name: "right-hand-rotation-acceleration-limit", val: getRotationAccelerationLimit(RIGHT_HAND_INDEX), checked: false},
            {name: "left-foot-translation-acceleration-limit", val: getTranslationAccelerationLimit(LEFT_FOOT_INDEX), checked: false},
            {name: "left-foot-rotation-acceleration-limit", val: getRotationAccelerationLimit(LEFT_FOOT_INDEX), checked: false},
            {name: "right-foot-translation-acceleration-limit", val: getTranslationAccelerationLimit(RIGHT_FOOT_INDEX), checked: false},
            {name: "right-foot-rotation-acceleration-limit", val: getRotationAccelerationLimit(RIGHT_FOOT_INDEX), checked: false},
            {name: "hips-translation-acceleration-limit", val: getTranslationAccelerationLimit(HIPS_INDEX), checked: false},
            {name: "hips-rotation-acceleration-limit", val: getRotationAccelerationLimit(HIPS_INDEX), checked: false},
            {name: "spine2-translation-acceleration-limit", val: getTranslationAccelerationLimit(SPINE2_INDEX), checked: false},
            {name: "spine2-rotation-acceleration-limit", val: getRotationAccelerationLimit(SPINE2_INDEX), checked: false}
        ];
        tablet.emitScriptEvent(JSON.stringify(values));
    } else if (msg.name === "left-hand-translation-acceleration-limit") {
        setTranslationAccelerationLimit(LEFT_HAND_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "left-hand-rotation-acceleration-limit") {
        setRotationAccelerationLimit(LEFT_HAND_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "right-hand-translation-acceleration-limit") {
        setTranslationAccelerationLimit(RIGHT_HAND_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "right-hand-rotation-acceleration-limit") {
        setRotationAccelerationLimit(RIGHT_HAND_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "left-foot-translation-acceleration-limit") {
        setTranslationAccelerationLimit(LEFT_FOOT_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "left-foot-rotation-acceleration-limit") {
        setRotationAccelerationLimit(LEFT_FOOT_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "right-foot-translation-acceleration-limit") {
        setTranslationAccelerationLimit(RIGHT_FOOT_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "right-foot-rotation-acceleration-limit") {
        setRotationAccelerationLimit(RIGHT_FOOT_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "hips-translation-acceleration-limit") {
        setTranslationAccelerationLimit(HIPS_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "hips-rotation-acceleration-limit") {
        setRotationAccelerationLimit(HIPS_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "spine2-translation-acceleration-limit") {
        setTranslationAccelerationLimit(SPINE2_INDEX, parseInt(msg.val, 10));
    } else if (msg.name === "spine2-rotation-acceleration-limit") {
        setRotationAccelerationLimit(SPINE2_INDEX, parseInt(msg.val, 10));
    }
}

tablet.screenChanged.connect(onScreenChanged);

function shutdownTabletApp() {
    tablet.removeButton(tabletButton);
    if (shown) {
        tablet.webEventReceived.disconnect(onWebEventReceived);
        tablet.gotoHomeScreen();
    }
    tablet.screenChanged.disconnect(onScreenChanged);
}

//
// end tablet app boiler plate
//

var mapping;
function mappingChanged() {
    if (mapping) {
        mapping.disable();
    }
    mapping = Controller.parseMapping(JSON.stringify(mappingJson));
    mapping.enable();
}

function shownChanged(newShown) {
    if (newShown) {
        mappingChanged();
    } else {
        mapping.disable();
    }
}

mappingChanged();

Script.scriptEnding.connect(function() {
    if (mapping) {
        mapping.disable();
    }
    tablet.removeButton(tabletButton);
});

