// shopReviewEntityScript.js
//
//  This script handles the review phase in the vrShop. It starts entering into the entity holding in hand the item to review.
//  Then the user can rate the item and record a review for that item.
//  Finally the recording is stored into the asset and a link to that file is stored into the DB entity of that item.
//  During the whole reviewing experience an (interactive) UI drives the user.

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var utilitiesScript = Script.resolvePath("../../libraries/utils.js");
    var overlayManagerScript = Script.resolvePath("../../libraries/overlayManager.js");
    Script.include(utilitiesScript);
    Script.include(overlayManagerScript);
        
    var POINTER_ICON_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/Pointer.png";
    var STAR_ON_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/SingleStar_Yellow.png";
    var STAR_OFF_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/SingleStar_Black.png";
    var RECORDING_ON_ICON_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/REC.png";
    var ANCHOR_ENTITY_FOR_UI_NAME = "anchorEntityForReviewUI";
    var START_RECORDING_TEXT = "Press the bumper to start recording";
    var STOP_RECORDING_TEXT = "Press the bumper to stop recording";
    var SHOPPING_CART_NAME = "Shopping cart";
    var CAMERA_NAME = "CameraReview";
    
    
    var RIGHT_HAND = 1;
    var LEFT_HAND = 0;
    
    var LINE_LENGTH = 100;
    var COLOR = {
        red: 165,
        green: 199,
        blue: 218
    };
    

    
    var _this;
    var dataBaseID = null;
    var cameraEntity = null;
    var scoreAssigned = null;
    var hoveredButton = null;
    var hoveredButtonIndex = -1;
    var recording = false;
    var workDone = false;

    var PENETRATION_THRESHOLD = 0.2;


    var isUIWorking = false;
    var wantToStopTrying = false;
    var rightController = null;
    var leftController = null;
    var workingHand = null;
    
    var mainPanel = null;
    var cameraPanel = null;
    var buttons = [];
    var onAirOverlay = null;
    var instructionsOverlay = null;
    
    
    var pointer = new Image3DOverlay({
        url: POINTER_ICON_URL,
        dimensions: {
            x: 0.015,
            y: 0.015
        },
        alpha: 1,
        emissive: true,
        isFacingAvatar: false,
        ignoreRayIntersection: true,
    })
    
    function ReviewZone() {
        _this = this;
        return;
    };
    
    function MyController(hand) {
        this.hand = hand;
        if (this.hand === RIGHT_HAND) {
            this.getHandPosition = MyAvatar.getRightPalmPosition;
            this.getHandRotation = MyAvatar.getRightPalmRotation;
            this.bumper = Controller.Standard.RB;
        } else {
            this.getHandPosition = MyAvatar.getLeftPalmPosition;
            this.getHandRotation = MyAvatar.getLeftPalmRotation;
            this.bumper = Controller.Standard.LB;
        }
        
        this.pickRay = null;        // ray object
        this.overlayLine = null;    // id of line overlay
        this.waitingForBumpReleased = false;
        
        this.overlayLineOn = function(closePoint, farPoint, color) {
            if (this.overlayLine == null) {
                var lineProperties = {
                    lineWidth: 5,
                    start: closePoint,
                    end: farPoint,
                    color: color,
                    ignoreRayIntersection: true,
                    visible: true,
                    alpha: 1
                };
                this.overlayLine = new Line3DOverlay(lineProperties);
            } else {
               this.overlayLine.start = closePoint;
               this.overlayLine.end = farPoint;
           }
        },
        
        this.updateRay = function() {
            //update the ray object
            this.pickRay = {
               origin: this.getHandPosition(),
               direction: Quat.getUp(this.getHandRotation())
            };
            //update the ray overlay and the pointer
            
            var rayPickResult = OverlayManager.findRayIntersection(this.pickRay);
            if (rayPickResult.intersects) {
                var normal = Vec3.multiply(Quat.getFront(Camera.getOrientation()), -1);
                var offset = Vec3.multiply(normal, 0.001);
                pointer.position =  Vec3.sum(rayPickResult.intersection, offset);       //pointer is a global Image3DOverlay
                pointer.rotation = Camera.getOrientation();
                pointer.visible = true;
            } else {
                pointer.visible = false;
            }
            
            var farPoint = rayPickResult.intersects ? rayPickResult.intersection : Vec3.sum(this.pickRay.origin, Vec3.multiply(this.pickRay.direction, LINE_LENGTH));
            this.overlayLineOn(this.pickRay.origin, farPoint, COLOR);
            
        },
        //the update of each hand has to update the ray belonging to that hand and handle the bumper event
        this.updateHand = function() {
            //detect the bumper event
            var bumperPressed = Controller.getValue(this.bumper);
            if (bumperPressed && this != workingHand) {
                //mantain active one ray at a time
                workingHand.clean();
                workingHand = this;
            } else if (this != workingHand) {
                return;
            }
            
            if (!scoreAssigned) {
                this.updateRay();
                
                //manage event on UI
                var lastHoveredButton = hoveredButton;
                hoveredButton = OverlayManager.findOnRay(this.pickRay);
                //print("hovered button: " + hoveredButton);
                if (lastHoveredButton != hoveredButton) {
                    hoveredButtonIndex = -1;
                    if (hoveredButton) {
                        for (var i = 0; i < buttons.length; i++) {
                            if (buttons[i] == hoveredButton) {
                                //print("Adapting overlay rendering");
                                hoveredButtonIndex = i;
                            }
                        }
                    }
                    adaptOverlayOnHover(hoveredButtonIndex);
                }
            } else if (!instructionsOverlay.visible) {
                workingHand.clean();
                for (var i = 0; i < buttons.length; i++) {
                    buttons[i].destroy();
                }
                buttons = [];
                instructionsOverlay.visible = true;
            }
            
            
            if (bumperPressed && !this.waitingForBumpReleased) {
                this.waitingForBumpReleased = true;
                
                if (hoveredButton) {
                    scoreAssigned = hoveredButtonIndex + 1;
                    hoveredButton = null;
                } else if (scoreAssigned && !recording) {
                    instructionsOverlay.text = STOP_RECORDING_TEXT;
                    Recording.startRecording();
                    onAirOverlay.visible = true;
                    recording = true;
                } else if (scoreAssigned && recording) {
                    Recording.stopRecording();
                    Recording.saveRecordingToAsset(saveDataIntoDB);
                    onAirOverlay.visible = false;
                    recording = false;
                    workDone = true;
                    _this.cleanUI();
                }
            } else if (!bumperPressed && this.waitingForBumpReleased) {
                this.waitingForBumpReleased = false;
            }
        },
        
        this.clean = function() {
            this.pickRay = null;
            if (this.overlayLine) {
                this.overlayLine.destroy();
            }
            this.overlayLine = null;
            pointer.visible = false;
        }
    };
    
    function update(deltaTime) {
        
        if (!workDone) {
            leftController.updateHand();
            rightController.updateHand();
        } else {
            _this.cleanUI();
            Script.update.disconnect(update);
        }
        
        
        if (!insideZone && isUIWorking) {
            // Destroy rays
            _this.cleanUI();
        }
        
    };
    
    //This method is the callback called once the recording is loaded into the asset
    function saveDataIntoDB(url) {
        // Feed the database
        var dbObj = getEntityCustomData('infoKey', dataBaseID, null);
        if(dbObj) {
            var myName = MyAvatar.displayName ? MyAvatar.displayName : "Anonymous";
            dbObj.dbKey[dbObj.dbKey.length] = {name: myName, score: scoreAssigned, clip_url: url};
            setEntityCustomData('infoKey', dataBaseID, dbObj);
            print("Feeded DB: " + url);
        }
    };
    
    // Find items in the zone. It returns a not null value if an item belonging to the user is found AND if a cart belonging to him is not found
    function findItemToReview(searchingPointEntityID) {
        var foundItemToReviewID = null;
        var entitiesInZone = Entities.findEntities(Entities.getEntityProperties(searchingPointEntityID).position, 5); 
        for (var i = 0; i < entitiesInZone.length; i++) {
            
            var ownerObj = getEntityCustomData('ownerKey', entitiesInZone[i], null);
            
            if (ownerObj) {
                print("Not sure if review. Check " + MyAvatar.sessionUUID);
                if (ownerObj.ownerID === MyAvatar.sessionUUID) {
                    if (Entities.getEntityProperties(entitiesInZone[i]).name == SHOPPING_CART_NAME) {
                        //the user has a cart, he's not a reviewer
                        print("the user has a cart, it's not a reviewer");
                        return null;
                    } else {
                        foundItemToReviewID = entitiesInZone[i];
                    }
                }
            }
        }
        var res = null;
        if (foundItemToReviewID) {
            res = Entities.getEntityProperties(foundItemToReviewID).name;
            print("Found an item to review: " + res);
            //delete the item
            Entities.deleteEntity(foundItemToReviewID);
        }
        return res;
    };
    
    function findAnchorEntityForUI(searchingPointEntityID) {
        
        var entitiesInZone = Entities.findEntities(Entities.getEntityProperties(searchingPointEntityID).position, 2); 
        for (var i = 0; i < entitiesInZone.length; i++) {
            
            if (Entities.getEntityProperties(entitiesInZone[i]).name == ANCHOR_ENTITY_FOR_UI_NAME) {
                print("Anchor entity found " + entitiesInZone[i]);
                return entitiesInZone[i];
            }
        }
        return null;
    };
    
    function findItemByName(searchingPointEntityID, itemName) {
        // find the database entity
        print("Looking for item: " + itemName);
        var entitiesInZone = Entities.findEntities(Entities.getEntityProperties(searchingPointEntityID).position, (Entities.getEntityProperties(searchingPointEntityID).dimensions.x)*10); 
        
        for (var i = 0; i < entitiesInZone.length; i++) {
            if (Entities.getEntityProperties(entitiesInZone[i]).name == itemName) {
                print(itemName + " found! " + entitiesInZone[i]);
                return entitiesInZone[i];
            }
        }
        print("Item " + itemName + " not found");
        return null;
    };
    
    function adaptOverlayOnHover(hoveredButtonIndex) {
        for (var i = buttons.length - 1; i >= 0; i--) {
            if (i <= hoveredButtonIndex) {
                buttons[i].url = STAR_ON_URL;
            } else {
                buttons[i].url = STAR_OFF_URL;
            }
        }
    };


    ReviewZone.prototype = {

        preload: function (entityID) {
        },
        
        //When the avatar comes into the review zone the script looks for the actual item to review, the proper DB for that item and the entity camera. If all goes well the UI is created
        enterEntity: function (entityID) {
            print("entering in the review area");
            insideZone = true;
            
            var itemToReview = findItemToReview(entityID); //return the name or null
            cameraEntity = findItemByName(entityID, CAMERA_NAME);
            if (itemToReview && cameraEntity) {
                dataBaseID = findItemByName(entityID, itemToReview + "DB"); //return the ID of the DB or null
                if (dataBaseID) {
                    var anchorEntityForUI = findAnchorEntityForUI(entityID);
                    if (anchorEntityForUI) {
                        _this.createReviewUI(anchorEntityForUI);
                        rightController = new MyController(RIGHT_HAND);     //rightController and leftController are two objects
                        leftController = new MyController(LEFT_HAND);
                        workingHand = rightController;
                        Script.update.connect(update);
                    }
                }
            }
        },

        leaveEntity: function (entityID) {
            print("leaving the review area");
            if (!workDone) {
                Script.update.disconnect(update);
            }
            if (recording) {
                Recording.stopRecording();
                recording = false;
            }
            
            if (cameraPanel) {
                cameraPanel.destroy();
                cameraPanel = null;
            }
            workDone = false;
            cameraEntity = null;
            dataBaseID = null;
            scoreAssigned = null;
            hoveredButton = null;
            hoveredButtonIndex = -1;
            insideZone = false;
            _this.cleanUI();
        },
        
        createReviewUI : function(anchorEntityID) {
            Entities.editEntity(anchorEntityID, { locked: false });
            var anchorEntityPosition = Entities.getEntityProperties(anchorEntityID).position;
            anchorEntityPosition.y = MyAvatar.getHeadPosition().y;
            Entities.editEntity(anchorEntityID, { position: anchorEntityPosition });
            Entities.editEntity(anchorEntityID, { locked: true });
            
            //set the main panel to follow the inspect entity
            mainPanel = new OverlayPanel({
                anchorPositionBinding: { entity: anchorEntityID },
                anchorRotationBinding: { entity: anchorEntityID },
                
                isFacingAvatar: false
            });
            
            var offsetPositionY = 0.0;
            var offsetPositionX = -0.3;
            
            for (var i = 0; i < 3; i++) {
                buttons[i] = new Image3DOverlay({
                    url: STAR_OFF_URL,
                    dimensions: {
                        x: 0.15,
                        y: 0.15
                    },
                    isFacingAvatar: false,
                    alpha: 0.8,
                    ignoreRayIntersection: false,
                    offsetPosition: {
                        x: offsetPositionX - (i * offsetPositionX),
                        y: offsetPositionY,
                        z: 0
                    },
                    emissive: true,
                });
                mainPanel.addChild(buttons[i]);
            }
            
            instructionsOverlay= new Text3DOverlay({
                    text: START_RECORDING_TEXT,
                    isFacingAvatar: false,
                    alpha: 1.0,
                    ignoreRayIntersection: true,
                    offsetPosition: {
                        x: -0.5,
                        y: 0,
                        z: 0
                    },
                    dimensions: { x: 0, y: 0 },
                    backgroundColor: { red: 0, green: 255, blue: 0 },
                    color: { red: 0, green: 0, blue: 0 },
                    topMargin: 0.00625,
                    leftMargin: 0.00625,
                    bottomMargin: 0.1,
                    rightMargin: 0.00625,
                    lineHeight: 0.06,
                    alpha: 1,
                    backgroundAlpha: 0.3,
                    visible: false
                });
                
            mainPanel.addChild(instructionsOverlay);
            
            cameraPanel = new OverlayPanel({
                anchorPositionBinding: { entity: cameraEntity },
                isFacingAvatar: false
            });
            
            onAirOverlay = new Image3DOverlay({
                    url: RECORDING_ON_ICON_URL,
                    dimensions: {
                        x: 0.2,
                        y: 0.2
                    },
                    isFacingAvatar: false,
                    alpha: 0.8,
                    ignoreRayIntersection: true,
                    offsetPosition: {
                        x: 0,
                        y: 0.7,
                        z: 0
                    },
                    emissive: true,
                    visible: false,
                });
            cameraPanel.addChild(onAirOverlay);
                
            isUIWorking = true;
        },
        
        cleanUI: function () {
            workingHand.clean();
            if (mainPanel) {
                mainPanel.destroy();
            }
            mainPanel = null;
            isUIWorking = false;
        },

        unload: function (entityID) {
            this.cleanUI();
            if (cameraPanel) {
                cameraPanel.destroy();
            }
        }
    }

    return new ReviewZone();
});