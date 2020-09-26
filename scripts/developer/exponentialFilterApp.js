var LEFT_HAND_INDEX = 0;
var RIGHT_HAND_INDEX = 1;
var LEFT_FOOT_INDEX = 2;
var RIGHT_FOOT_INDEX = 3;
var HIPS_INDEX = 4;
var SPINE2_INDEX = 5;

var HAND_SMOOTHING_TRANSLATION = 0.3;
var HAND_SMOOTHING_ROTATION = 0.15;
var FOOT_SMOOTHING_TRANSLATION = 0.3;
var FOOT_SMOOTHING_ROTATION = 0.15;
var TORSO_SMOOTHING_TRANSLATION = 0.3;
var TORSO_SMOOTHING_ROTATION = 0.16;

var mappingJson = {
    name: "com.highfidelity.testing.exponentialFilterApp",
    channels: [
        {
            from: "Standard.LeftHand",
            to: "Actions.LeftHand",
            filters: [
                {
                    type: "exponentialSmoothing",
                    translation: HAND_SMOOTHING_TRANSLATION,
                    rotation: HAND_SMOOTHING_ROTATION
                }
            ]
        },
        {
            from: "Standard.RightHand",
            to: "Actions.RightHand",
            filters: [
                {
                    type: "exponentialSmoothing",
                    translation: HAND_SMOOTHING_TRANSLATION,
                    rotation: HAND_SMOOTHING_ROTATION
                }
            ]
        },
        {
            from: "Standard.LeftFoot",
            to: "Actions.LeftFoot",
            filters: [
                {
                    type: "exponentialSmoothing",
                    translation: FOOT_SMOOTHING_TRANSLATION,
                    rotation: FOOT_SMOOTHING_ROTATION
                }
            ]
        },
        {
            from: "Standard.RightFoot",
            to: "Actions.RightFoot",
            filters: [
                {
                    type: "exponentialSmoothing",
                    translation: FOOT_SMOOTHING_TRANSLATION,
                    rotation: FOOT_SMOOTHING_ROTATION
                }
            ]
        },
        {
            from: "Standard.Hips",
            to: "Actions.Hips",
            filters: [
                {
                    type: "exponentialSmoothing",
                    translation: TORSO_SMOOTHING_TRANSLATION,
                    rotation: TORSO_SMOOTHING_ROTATION
                }
            ]
        },
        {
            from: "Standard.Spine2",
            to: "Actions.Spine2",
            filters: [
                {
                    type: "exponentialSmoothing",
                    translation: TORSO_SMOOTHING_TRANSLATION,
                    rotation: TORSO_SMOOTHING_ROTATION
                }
            ]
        }
    ]
};

//
// tablet app boiler plate
//

var TABLET_BUTTON_NAME = "EXPFILT";
var HTML_URL = Script.getExternalPath(Script.ExternalPaths.HF_Public, "/tony/html/exponentialFilterApp.html?7");

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

function getTranslation(i) {
    return mappingJson.channels[i].filters[0].translation;
}
function setTranslation(i, value) {
    mappingJson.channels[i].filters[0].translation = value;
    mappingChanged();
}
function getRotation(i) {
    return mappingJson.channels[i].filters[0].rotation;
}
function setRotation(i, value) {
    mappingJson.channels[i].filters[0].rotation = value; mappingChanged();
}

function onWebEventReceived(msg) {
    if (msg.name === "init-complete") {
        var values = [
            {name: "enable-filtering", val: filterEnabled ? "on" : "off", checked: false},
            {name: "left-hand-translation", val: getTranslation(LEFT_HAND_INDEX), checked: false},
            {name: "left-hand-rotation", val: getRotation(LEFT_HAND_INDEX), checked: false},
            {name: "right-hand-translation", val: getTranslation(RIGHT_HAND_INDEX), checked: false},
            {name: "right-hand-rotation", val: getRotation(RIGHT_HAND_INDEX), checked: false},
            {name: "left-foot-translation", val: getTranslation(LEFT_FOOT_INDEX), checked: false},
            {name: "left-foot-rotation", val: getRotation(LEFT_FOOT_INDEX), checked: false},
            {name: "right-foot-translation", val: getTranslation(RIGHT_FOOT_INDEX), checked: false},
            {name: "right-foot-rotation", val: getRotation(RIGHT_FOOT_INDEX), checked: false},
            {name: "hips-translation", val: getTranslation(HIPS_INDEX), checked: false},
            {name: "hips-rotation", val: getRotation(HIPS_INDEX), checked: false},
            {name: "spine2-translation", val: getTranslation(SPINE2_INDEX), checked: false},
            {name: "spine2-rotation", val: getRotation(SPINE2_INDEX), checked: false}
        ];
        tablet.emitScriptEvent(JSON.stringify(values));
    } else if (msg.name === "enable-filtering") {
        if (msg.val === "on") {
            filterEnabled = true;
        } else if (msg.val === "off") {
            filterEnabled = false;
        }
        mappingChanged();
    } else if (msg.name === "left-hand-translation") {
        setTranslation(LEFT_HAND_INDEX, Number(msg.val));
    } else if (msg.name === "left-hand-rotation") {
        setRotation(LEFT_HAND_INDEX, Number(msg.val));
    } else if (msg.name === "right-hand-translation") {
        setTranslation(RIGHT_HAND_INDEX, Number(msg.val));
    } else if (msg.name === "right-hand-rotation") {
        setRotation(RIGHT_HAND_INDEX, Number(msg.val));
    } else if (msg.name === "left-foot-translation") {
        setTranslation(LEFT_FOOT_INDEX, Number(msg.val));
    } else if (msg.name === "left-foot-rotation") {
        setRotation(LEFT_FOOT_INDEX, Number(msg.val));
    } else if (msg.name === "right-foot-translation") {
        setTranslation(RIGHT_FOOT_INDEX, Number(msg.val));
    } else if (msg.name === "right-foot-rotation") {
        setRotation(RIGHT_FOOT_INDEX, Number(msg.val));
    } else if (msg.name === "hips-translation") {
        setTranslation(HIPS_INDEX, Number(msg.val));
    } else if (msg.name === "hips-rotation") {
        setRotation(HIPS_INDEX, Number(msg.val));
    } else if (msg.name === "spine2-translation") {
        setTranslation(SPINE2_INDEX, Number(msg.val));
    } else if (msg.name === "spine2-rotation") {
        setRotation(SPINE2_INDEX, Number(msg.val));
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

var filterEnabled = true;
var mapping;
function mappingChanged() {
    if (mapping) {
        mapping.disable();
    }
    if (filterEnabled) {
        mapping = Controller.parseMapping(JSON.stringify(mappingJson));
        mapping.enable();
    }
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

