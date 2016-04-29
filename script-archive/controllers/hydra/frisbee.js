//
//  frisbee.js
//  examples
//
//  Created by Thijs Wenker on 7/5/14.
//  Copyright 2014 High Fidelity, Inc.
//  
//  Requirements: Razer Hydra's
//
//  Fun game to throw frisbee's to eachother. Hold the trigger on any of the hydra's to create or catch a frisbee.
//
//  Tip: use this together with the squeezeHands.js script to make it look nicer.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
Script.include("../../libraries/toolBars.js");

const LEFT_PALM = 0;
const LEFT_TIP = 1;
const LEFT_BUTTON_FWD = 5;
const LEFT_BUTTON_3 = 3;

const RIGHT_PALM = 2;
const RIGHT_TIP = 3;
const RIGHT_BUTTON_FWD = 11;
const RIGHT_BUTTON_3 = 9;

const FRISBEE_RADIUS = 0.08;
const GRAVITY_STRENGTH = 0.5;

const CATCH_RADIUS = 0.5;
const MIN_SIMULATION_SPEED = 0.15;
const THROWN_VELOCITY_SCALING = 1.5;

const SOUNDS_ENABLED = true;
const FRISBEE_BUTTON_URL = "http://test.thoys.nl/hifi/images/frisbee/frisbee_button_by_Judas.svg";
const FRISBEE_MODEL_SCALE = 275;
const FRISBEE_MENU = "Toys>Frisbee";
const FRISBEE_DESIGN_MENU = "Toys>Frisbee>Design";
const FRISBEE_ENABLED_SETTING = "Frisbee>Enabled";
const FRISBEE_CREATENEW_SETTING = "Frisbee>CreateNew";
const FRISBEE_DESIGN_SETTING = "Frisbee>Design";
const FRISBEE_FORCE_MOUSE_CONTROLS_SETTING = "Frisbee>ForceMouseControls";

//Add your own designs in FRISBEE_DESIGNS, be sure to put frisbee in the URL if you want others to be able to catch it without having a copy of your frisbee script.
const FRISBEE_DESIGNS = [
                          {"name":"Interface", "model":"http://test.thoys.nl/hifi/models/frisbee/frisbee.fbx"},
                          {"name":"Pizza", "model":"http://test.thoys.nl/hifi/models/frisbee/pizza.fbx"},
                          {"name":"Swirl", "model":"http://test.thoys.nl/hifi/models/frisbee/swirl.fbx"},
                          {"name":"Mayan", "model":"http://test.thoys.nl/hifi/models/frisbee/mayan.fbx"},
                        ];
const FRISBEE_MENU_DESIGN_POSTFIX = " Design";
const FRISBEE_DESIGN_RANDOM = "Random";

const SPIN_MULTIPLIER = 1000;
const FRISBEE_LIFETIME = 300; // 5 minutes

var windowDimensions = Controller.getViewportDimensions();
var toolHeight = 50;
var toolWidth = 50;
var frisbeeToggle;
var toolBar;
var frisbeeEnabled = true;
var newfrisbeeEnabled = false;
var forceMouseControls = false;
var hydrasConnected = false;
var selectedDesign = FRISBEE_DESIGN_RANDOM;

function loadSettings() {
    frisbeeEnabled = Settings.getValue(FRISBEE_ENABLED_SETTING, "true") == "true";
    newfrisbeeEnabled = Settings.getValue(FRISBEE_CREATENEW_SETTING, "false") == "true";
    forceMouseControls = Settings.getValue(FRISBEE_FORCE_MOUSE_CONTROLS_SETTING, "false") == "true";
    selectedDesign = Settings.getValue(FRISBEE_DESIGN_SETTING, "Random");
}

function saveSettings() {
    Settings.setValue(FRISBEE_ENABLED_SETTING, frisbeeEnabled ? "true" : "false");
    Settings.setValue(FRISBEE_CREATENEW_SETTING, newfrisbeeEnabled ? "true" : "false");
    Settings.setValue(FRISBEE_FORCE_MOUSE_CONTROLS_SETTING, forceMouseControls ? "true" : "false");
    Settings.setValue(FRISBEE_DESIGN_SETTING, selectedDesign); 
}

function moveOverlays() {
    var newViewPort = Controller.getViewportDimensions();
    if (typeof(toolBar) === 'undefined') {
        initToolBar();
    } else if (windowDimensions.x == newViewPort.x &&
               windowDimensions.y == newViewPort.y) {
        return;
    }
    
    windowDimensions = newViewPort;
    var toolsX = windowDimensions.x - 8 - toolBar.width;
    var toolsY = (windowDimensions.y - toolBar.height) / 2 + 80;    
    toolBar.move(toolsX, toolsY);
}

function frisbeeURL() {
    return selectedDesign == FRISBEE_DESIGN_RANDOM ? FRISBEE_DESIGNS[Math.floor(Math.random() * FRISBEE_DESIGNS.length)].model : getFrisbee(selectedDesign).model;
}

//This function checks if the modelURL is inside of our Designs or contains "frisbee" in it.
function validFrisbeeURL(frisbeeURL) {
    for (var frisbee in FRISBEE_DESIGNS) {
        if (FRISBEE_DESIGNS[frisbee].model == frisbeeURL) {
            return true;
        }
    }
    return frisbeeURL.toLowerCase().indexOf("frisbee") !== -1;
}

function getFrisbee(frisbeeName) {
    for (var frisbee in FRISBEE_DESIGNS) {
        if (FRISBEE_DESIGNS[frisbee].name == frisbeeName) {
            return FRISBEE_DESIGNS[frisbee];
        }
    }
    return undefined;
}

function Hand(name, palm, tip, forwardButton, button3, trigger) {
    this.name = name;
    this.palm = palm;
    this.tip = tip;
    this.forwardButton = forwardButton;
    this.button3 = button3;
    this.trigger = trigger;
    this.holdingFrisbee = false;
    this.entity = false;
    this.palmPosition = function () {
        return this.palm == LEFT_PALM ? MyAvatar.leftHandPose.translation : MyAvatar.rightHandPose.translation;
    };

    this.grabButtonPressed = function() { 
                                    return (
                                            Controller.getValue(this.forwardButton)  ||
                                            Controller.getValue(this.button3) ||
                                            Controller.getValue(this.trigger) > 0.5
                                            )
                                        };
    this.holdPosition = function () {
        return this.palm == LEFT_PALM ? MyAvatar.leftHandPose.translation : MyAvatar.rightHandPose.translation;
    };

    this.holdRotation = function() {
        var q = (this.palm == LEFT_PALM) ? Controller.getPoseValue(Controller.Standard.leftHand).rotation
                                         : Controller.getPoseValue(Controller.Standard.rightHand).rotation;
        q = Quat.multiply(MyAvatar.orientation, q);
        return {x: q.x, y: q.y, z: q.z, w: q.w};
    };
    this.tipVelocity = function () {
        return this.tip == LEFT_TIP ? MyAvatar.leftHandTipPose.velocity : MyAvatar.rightHandTipPose.velocity;
    };
}

function MouseControl(button) {
    this.button = button;
}

var leftHand = new Hand("LEFT", LEFT_PALM, LEFT_TIP, Controller.Standard.LB, Controller.Standard.LeftPrimaryThumb, Controller.Standard.LT);
var rightHand = new Hand("RIGHT", RIGHT_PALM, RIGHT_TIP, Controller.Standard.RB, Controller.Standard.RightPrimaryThumb, Controller.Standard.RT);

var leftMouseControl = new MouseControl("LEFT");
var middleMouseControl = new MouseControl("MIDDLE");
var rightMouseControl = new MouseControl("RIGHT");
var mouseControls = [leftMouseControl, middleMouseControl, rightMouseControl];
var currentMouseControl = false;

var newSound = SoundCache.getSound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/throw.raw");
var catchSound = SoundCache.getSound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/catch.raw");
var throwSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Switches%20and%20sliders/slider%20-%20whoosh1.raw");

var simulatedFrisbees = [];

var wantDebugging = false;
function debugPrint(message) {
    if (wantDebugging) {
        print(message);
    }
}

function playSound(sound, position) {
    if (!SOUNDS_ENABLED) {
        return;
    }
    
    Audio.playSound(sound,{
      position: position
    });
}

function cleanupFrisbees() {
    simulatedFrisbees = [];
    var entities = Entities.findEntities(MyAvatar.position, 1000);
    for (entity in entities) {
        Entities.deleteEntity(entities[entity]);
    }
}

function checkControllerSide(hand) {
 //   print("cCS");
    // If I don't currently have a frisbee in my hand, then try to catch closest one
    if (!hand.holdingFrisbee && hand.grabButtonPressed()) {
        var closestEntity = Entities.findClosestEntity(hand.palmPosition(), CATCH_RADIUS);
        var modelUrl = Entities.getEntityProperties(closestEntity).modelURL;
        print("lol2"+closestEntity.isKnownID);
        if (closestEntity && validFrisbeeURL(Entities.getEntityProperties(closestEntity).modelURL)) {
            print("lol");
            Entities.editEntity(closestEntity, {modelScale: 1, inHand: true, position: hand.holdPosition(), shouldDie: true});
            Entities.deleteEntity(closestEntity);
            debugPrint(hand.message + " HAND- CAUGHT SOMETHING!!");
            print("lol");
            var properties = {
                type: "Model",
                position: hand.holdPosition(), 
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                dimensions: { x: FRISBEE_RADIUS, y: FRISBEE_RADIUS / 5, z: FRISBEE_RADIUS },
                damping: 0.999,
                modelURL: modelUrl,
                scale: FRISBEE_MODEL_SCALE,
                rotation: hand.holdRotation(),
                lifetime: FRISBEE_LIFETIME
            };

            newEntity = Entities.addEntity(properties);
            
            hand.holdingFrisbee = true;
            hand.entity = newEntity;

            playSound(catchSound, hand.holdPosition());
            
            return; // exit early
        }
    }

    //  If '3' is pressed, and not holding a frisbee, make a new one
    if (hand.grabButtonPressed() && !hand.holdingFrisbee && newfrisbeeEnabled) {
        var properties = {
                type: "Model",
                position: hand.holdPosition(), 
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                dimensions: { x: FRISBEE_RADIUS, y: FRISBEE_RADIUS / 5, z: FRISBEE_RADIUS },
                damping: 0,
                modelURL: frisbeeURL(),
                scale: FRISBEE_MODEL_SCALE,
                rotation: hand.holdRotation(),
                lifetime: FRISBEE_LIFETIME
            };

        newEntity = Entities.addEntity(properties);
        hand.holdingFrisbee = true;
        hand.entity = newEntity;

        // Play a new frisbee sound
        playSound(newSound, hand.holdPosition());
        
        return; // exit early
    }

    if (hand.holdingFrisbee) {
        //  If holding the frisbee keep it in the palm
        if (hand.grabButtonPressed()) {
            debugPrint(">>>>> " + hand.name + "-FRISBEE IN HAND, grabbing, hold and move");
            var properties = { 
                    position: hand.holdPosition(),
                    modelRotation: hand.holdRotation()
                };
            Entities.editEntity(hand.entity, properties);
        } else {
            debugPrint(">>>>> " + hand.name + "-FRISBEE IN HAND, not grabbing, THROW!!!");
            //  If frisbee just released, add velocity to it!
            
            var properties = {
                    velocity: Vec3.multiply(hand.tipVelocity(), THROWN_VELOCITY_SCALING),
                    inHand: false,
                    lifetime: FRISBEE_LIFETIME,
                    gravity: { x: 0, y: -GRAVITY_STRENGTH, z: 0},
                    rotation: hand.holdRotation()
                };

            Entities.editEntity(hand.entity, properties);

            simulatedFrisbees.push(hand.entity);

            hand.holdingFrisbee = false;
            hand.entity = false;
            
            playSound(throwSound, hand.holdPosition());
        }
    }
}

function initToolBar() {
    toolBar = new ToolBar(0, 0, ToolBar.VERTICAL);
    frisbeeToggle = toolBar.addTool({
                               imageURL: FRISBEE_BUTTON_URL,
                               subImage: { x: 0, y: Tool.IMAGE_WIDTH, width: Tool.IMAGE_WIDTH, height: Tool.IMAGE_HEIGHT },
                               width: toolWidth,
                               height: toolHeight,
                               visible: true,
                               alpha: 0.9
                               }, true);
    enableNewFrisbee(newfrisbeeEnabled);
}

function hydraCheck() {
    return Controller.Hardware.Hydra !== undefined;
}

function checkController(deltaTime) {
    moveOverlays();
    if (!frisbeeEnabled) {
        return;
    }
    // this is expected for hydras
    if (hydraCheck()) {
///print("testrr ");
        checkControllerSide(leftHand);
        checkControllerSide(rightHand);
    }
    if (!hydrasConnected || forceMouseControls) {
        //TODO: add mouse cursor control code here.
    }
}

function controlFrisbees(deltaTime) {
    var killSimulations = [];
    for (frisbee in simulatedFrisbees) {
        var properties = Entities.getEntityProperties(simulatedFrisbees[frisbee]);
        //get the horizon length from the velocity origin in order to get speed
        var speed = Vec3.length({x:properties.velocity.x, y:0, z:properties.velocity.z});
        if (speed < MIN_SIMULATION_SPEED) {
            //kill the frisbee simulation when speed is low
            killSimulations.push(frisbee);
            continue;
        }
        Entities.editEntity(simulatedFrisbees[frisbee], {rotation: Quat.multiply(properties.modelRotation, Quat.fromPitchYawRollDegrees(0, speed * deltaTime * SPIN_MULTIPLIER, 0))});
        
    }
    for (var i = killSimulations.length - 1; i >= 0; i--) {
        simulatedFrisbees.splice(killSimulations[i], 1);
    }
}

//catches interfering calls of hydra-cursors
function withinBounds(coords) {
    return coords.x >= 0 && coords.x < windowDimensions.x && coords.y >= 0 && coords.y < windowDimensions.y;    
}

function mouseMoveEvent(event) {
    //TODO: mouse controls //print(withinBounds(event)); //print("move"+event.x);
}

function mousePressEvent(event) {
    print(event.x);
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    if (frisbeeToggle == toolBar.clicked(clickedOverlay)) {
        newfrisbeeEnabled = !newfrisbeeEnabled;
        saveSettings();
        enableNewFrisbee(newfrisbeeEnabled);
    }
}

function enableNewFrisbee(enable) {
    if (toolBar.numberOfTools() > 0) {
        toolBar.tools[0].select(enable);
    }
}

function mouseReleaseEvent(event) {
    //TODO: mouse controls //print(JSON.stringify(event));
}

function setupMenus() {
    Menu.addMenu(FRISBEE_MENU);
    Menu.addMenuItem({ 
        menuName: FRISBEE_MENU, 
        menuItemName: "Frisbee Enabled",
        isCheckable: true,
        isChecked: frisbeeEnabled
    });
    Menu.addMenuItem({ 
        menuName: FRISBEE_MENU, 
        menuItemName: "Cleanup Frisbees"
    });
    Menu.addMenuItem({ 
        menuName: FRISBEE_MENU, 
        menuItemName: "Force Mouse Controls",
        isCheckable: true,
        isChecked: forceMouseControls
    });
    Menu.addMenu(FRISBEE_DESIGN_MENU);
    Menu.addMenuItem({
        menuName: FRISBEE_DESIGN_MENU, 
        menuItemName: FRISBEE_DESIGN_RANDOM + FRISBEE_MENU_DESIGN_POSTFIX,
        isCheckable: true,
        isChecked: selectedDesign == FRISBEE_DESIGN_RANDOM
    });
    for (frisbee in FRISBEE_DESIGNS) {
        Menu.addMenuItem({ 
            menuName: FRISBEE_DESIGN_MENU, 
            menuItemName: FRISBEE_DESIGNS[frisbee].name + FRISBEE_MENU_DESIGN_POSTFIX,
            isCheckable: true,
            isChecked: selectedDesign == FRISBEE_DESIGNS[frisbee].name
        });
    }
}

//startup calls:
loadSettings();
setupMenus();
function scriptEnding() {
    toolBar.cleanup();
    Menu.removeMenu(FRISBEE_MENU);
}

function menuItemEvent(menuItem) {
    if (menuItem == "Cleanup Frisbees") {
        cleanupFrisbees();
        return;
    } else if (menuItem == "Frisbee Enabled") {
        frisbeeEnabled = Menu.isOptionChecked(menuItem);
        saveSettings();
        return;
    } else if (menuItem == "Force Mouse Controls") {
        forceMouseControls = Menu.isOptionChecked(menuItem);
        saveSettings();
        return;
    }
    if (menuItem.indexOf(FRISBEE_MENU_DESIGN_POSTFIX, menuItem.length - FRISBEE_MENU_DESIGN_POSTFIX.length) !== -1) {
        var item_name = menuItem.substring(0, menuItem.length - FRISBEE_MENU_DESIGN_POSTFIX.length);
        if (item_name == FRISBEE_DESIGN_RANDOM || getFrisbee(item_name) != undefined) {
            Menu.setIsOptionChecked(selectedDesign + FRISBEE_MENU_DESIGN_POSTFIX, false);
            selectedDesign = item_name;
            saveSettings();
            Menu.setIsOptionChecked(selectedDesign + FRISBEE_MENU_DESIGN_POSTFIX, true);
        }
    }
}

// register the call back so it fires before each data send
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.mouseReleaseEvent.connect(mouseReleaseEvent);
Menu.menuItemEvent.connect(menuItemEvent);
Script.scriptEnding.connect(scriptEnding);
Script.update.connect(checkController);
Script.update.connect(controlFrisbees);
