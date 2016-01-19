// shopInspectEntityScript.js
//
//  The inspection entity which runs this entity script will be in fron of the avatar while he's grabbing an item.
//  This script gives some information to the avatar using interactive 3DOverlays:
//  - Drive the customer to put he item in the correct zone to inspect it
//  - Create the UI while inspecting with text and buttons and manages the clicks on them modifying the item and communicating with agents
//  And also it creates the MyController which are in charge to render the rays from the hands during inspecting and analysing their intersection with other overlays

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function() {
    
    
    var utilitiesScript = Script.resolvePath("../../libraries/utils.js");
    var overlayManagerScript = Script.resolvePath("../../libraries/overlayManager.js");
    Script.include(utilitiesScript);
    Script.include(overlayManagerScript);
    
    var AGENT_REVIEW_CHANNEL = "reviewChannel";
    var NO_REVIEWS_AVAILABLE = "No reviews available";
    var SEPARATOR = "Separator";
    var CAMERA_REVIEW = "CameraReview";
    
    var ZERO_STAR_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/0Star.png";
    var ONE_STAR_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/1Star.png";
    var TWO_STAR_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/2Star.png";
    var THREE_STAR_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/3Star.png";
    
    var POINTER_ICON_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/Pointer.png";
    var TRY_ON_ICON = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/TryOn.png"
    
    var MIN_DIMENSION_THRESHOLD = null;
    var IN_HAND_STATUS = "inHand";
    var IN_INSPECT_STATUS = "inInspect";
    
    var RIGHT_HAND = 1;
    var LEFT_HAND = 0;
    
    var LINE_LENGTH = 100;
    var COLOR = {
        red: 165,
        green: 199,
        blue: 218
    };
    
    var COMFORT_ARM_LENGTH = 0.4;
    
    var PENETRATION_THRESHOLD = 0.2;

    var _this;
    var inspecting = false;
    var inspectingMyItem = false;
    var inspectedEntityID = null;
    var isUIWorking = false;
    var wantToStopTrying = false;
    var rightController = null;
    var leftController = null;
    var workingHand = null;
    var collidedItemID = null;
    var tempTryEntity = null;
    var tryingOnAvatar = false;
    var itemOriginalDimensions = null;
    
    var mainPanel = null;
    var mirrorPanel = null;
    var buttons = [];
    var tryOnAvatarButton = null;
    var playButton = null;
    var nextButton = null;
    var textReviewerName = null;
    var modelURLsArray = [];
    var previewURLsArray = [];
    var starURL = null;
    
    var reviewIndex = 0;
    var reviewsNumber = 0;
    var dbMatrix = null;
    
    var separator = null;
    var cameraReview = null;
    
    
    var pointer = new Image3DOverlay({          //maybe we want to use one pointer for each hand ?
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


    
    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    InspectEntity = function() {
         _this = this;
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
                workingHand.clean();
                workingHand = this;
            } else if (this != workingHand) {
                return;
            }
            
            this.updateRay();
            
            //manage event on UI
            if (bumperPressed && !this.waitingForBumpReleased) {
                this.waitingForBumpReleased = true;     //to avoid looping on the button while keep pressing the bumper
                var triggeredButton = OverlayManager.findOnRay(this.pickRay);
                if (triggeredButton != null) {
                    //search the index of the UI element triggered
                    for (var i = 0; i < buttons.length; i++) {
                        if (buttons[i] == triggeredButton) {
                            
                            _this.changeModel(i);
                        }
                    }
                    
                    //the nextButton moves to the next customer review, changing the agent accordingly
                    if (nextButton == triggeredButton) {
                        
                        reviewIndex ++;
                        if (reviewIndex == reviewsNumber) {
                            reviewIndex = 0;
                        }
                        
                        var message = {
                            command: "Show",
                            clip_url: dbMatrix[reviewIndex].clip_url
                        };
                        
                        Messages.sendMessage(AGENT_REVIEW_CHANNEL, JSON.stringify(message));
                        print("Show sent to agent");
                        
                        // update UI
                        textReviewerName.text = dbMatrix[reviewIndex].name;
                        reviewerScore.url = starConverter(dbMatrix[reviewIndex].score);
                        print("UI updated");
                    }
                    
                    //the playButton sends the play command to the agent
                    if (playButton == triggeredButton) {
                         var message = {
                            command: "Play",
                            clip_url: dbMatrix[reviewIndex].clip_url
                        };
                        
                        Messages.sendMessage(AGENT_REVIEW_CHANNEL, JSON.stringify(message));
                        print("Play sent to agent");
                        
                    }
                    
                    //the tryOnAvatarButton change the camera mode and puts t a copy of the inspected item in a proper position on the the avatar
                    if (tryOnAvatarButton == triggeredButton) {
                        print("tryOnAvatar pressed!");
                        
                        var itemPositionWhileTrying = null;
                        //All the offset here are good just for Will avatar increasing its size by one
                        switch (Entities.getEntityProperties(inspectedEntityID).name) {
                            case "Item_Sunglasses":
                                itemPositionWhileTrying = {x: 0, y: 0.04, z: 0.05};
                                break;
                            case "Item_Hat":
                                itemPositionWhileTrying = {x: 0, y: 0.16, z: 0.025};
                                break;
                            default:
                                //there isn't any position specified for that item, use a default one
                                itemPositionWhileTrying = {x: 0, y: 0.16, z: 0.025};
                                break;
                        }
                        
                        //Code for the overlay text for the mirror.
                        mirrorPanel = new OverlayPanel({
                            anchorPositionBinding: { avatar: "MyAvatar" },
                            anchorRotationBinding: { avatar: "MyAvatar" },
                                offsetPosition: {
                                x: 0.5,
                                y: 0.9,
                                z: 0
                            },
                            offsetRotation: Quat.fromVec3Degrees({x: 0, y: 180, z: 0}),
                            
                            isFacingAvatar: false
                        });
                        var mirrorText = new Text3DOverlay({
                            text: "Press the bumper to go back in inspection",
                            isFacingAvatar: false,
                            ignoreRayIntersection: true,

                            
                            dimensions: { x: 0, y: 0 },
                            backgroundColor: { red: 255, green: 255, blue: 255 },
                            color: { red: 200, green: 0, blue: 0 },
                            topMargin: 0.00625,
                            leftMargin: 0.00625,
                            bottomMargin: 0.1,
                            rightMargin: 0.00625,
                            lineHeight: 0.05,
                            alpha: 1,
                            backgroundAlpha: 0.3,
                            visible: true
                        });
                        mirrorPanel.addChild(mirrorText);
                        
                        tryingOnAvatar = true;
                        
                        //Clean inspect Overlays and related stuff
                        workingHand.clean();
                        mainPanel.destroy();
                        mainPanel = null;
                        isUIWorking = false;
                        Entities.editEntity(inspectedEntityID, { visible: false });     //the inspected item becomes invisible
                        
                        
                        Camera.mode = "entity";
                        Camera.cameraEntity = _this.entityID;
                        var entityProperties = Entities.getEntityProperties(inspectedEntityID);
                        tempTryEntity = Entities.addEntity({
                            type: entityProperties.type,
                            name: entityProperties.name,
                            localPosition: itemPositionWhileTrying,
                            dimensions: itemOriginalDimensions,
                            collisionsWillMove: false,
                            ignoreForCollisions: true,
                            modelURL: entityProperties.modelURL,
                            shapeType: entityProperties.shapeType,
                            originalTextures: entityProperties.originalTextures,
                            parentID: MyAvatar.sessionUUID,
                            parentJointIndex: MyAvatar.getJointIndex("Head")
                        });
                    }
                }
            } else if (!bumperPressed && this.waitingForBumpReleased) {
                this.waitingForBumpReleased = false;
            }
        },
        
        this.clean = function() {
            this.pickRay = null;
            this.overlayLine.destroy();
            this.overlayLine = null;
            pointer.visible = false;
        }
    };
    
    function update(deltaTime) {
        
        if (tryingOnAvatar) {
            // if trying the item on avatar, wait for a bumper being pressed to exit this mode
            //the bumper is already pressed when we get here because we triggered the button pressing the bumper so we have to wait it's released
            if(Controller.getValue(workingHand.bumper) && wantToStopTrying) {
                Camera.cameraEntity = null;
                Camera.mode = "first person";
                mirrorPanel.destroy();
                mirrorPanel = null;
                Entities.deleteEntity(tempTryEntity);
                tempTryEntity = null;
                Entities.editEntity(inspectedEntityID, { visible: true });
                _this.createInspectUI();
                tryingOnAvatar = false;
                wantToStopTrying = false;
            } else if (!Controller.getValue(workingHand.bumper) && !wantToStopTrying) {
                //no bumper is pressed
                wantToStopTrying = true;
            }
            return;
        } else if (inspecting) {
            //update the rays from both hands
            leftController.updateHand();
            rightController.updateHand();

            //check the item status for consistency
            var entityStatus = getEntityCustomData('statusKey', inspectedEntityID, null).status;
            if (entityStatus == IN_HAND_STATUS) {
                //the inspection is over
                inspecting = false;
                inspectedEntityID = null;
            }
        } else if (isUIWorking) {
            //getting here when the inspect phase end, so we want to clean the UI
            // Destroy rays
            workingHand.clean();
            
            // Destroy overlay
            mainPanel.destroy();
            isUIWorking = false;
            
            var message = {
                            command: "Hide",
                            clip_url: ""
                        };
                        
            Messages.sendMessage(AGENT_REVIEW_CHANNEL, JSON.stringify(message));
            print("Hide sent to agent");
            
            if (separator != null || cameraReview != null) {
                Entities.editEntity(separator, { visible: true });
                Entities.editEntity(separator, { locked: true });
                Entities.editEntity(cameraReview, { visible: true });
                Entities.editEntity(cameraReview, { locked: true });
            }
        }
        
        _this.positionRotationUpdate();
    };
    
    function starConverter(value) {
        var starURL = ZERO_STAR_URL;
        
        switch(value) {
            case 0:
                starURL = ZERO_STAR_URL;
                break;
                
            case 1:
                starURL = ONE_STAR_URL;
                break;
                
            case 2:
                starURL = TWO_STAR_URL;
                break;
                
            case 3:
                starURL = THREE_STAR_URL;
                break;
                
            default:
                starURL = ZERO_STAR_URL;
                break;
        }
                
        return starURL;
    };
    
    
    // look for the database entity relative to the inspected item
    function findItemDataBase(entityID, item) {
        var dataBaseID = null;
        var databaseEntityName = item + "DB";
        var entitiesInZone = Entities.findEntities(Entities.getEntityProperties(entityID).position, (Entities.getEntityProperties(entityID).dimensions.x)*100); 
        
        for (var i = 0; i < entitiesInZone.length && dataBaseID == null; i++) {
            if (Entities.getEntityProperties(entitiesInZone[i]).name == databaseEntityName) {
                dataBaseID = entitiesInZone[i];
                print("Database found! " + entitiesInZone[i]);
                return dataBaseID;
            }
        }
        print("No database for this item.");
        return null;
    };
    
    function findItemByName(searchingPointEntityID, itemName) {
        print("Looking for item: " + itemName);
        var entitiesInZone = Entities.findEntities(Entities.getEntityProperties(searchingPointEntityID).position, (Entities.getEntityProperties(searchingPointEntityID).dimensions.x)*100); 
        
        for (var i = 0; i < entitiesInZone.length; i++) {
            if (Entities.getEntityProperties(entitiesInZone[i]).name == itemName) {
                print(itemName + " found! " + entitiesInZone[i]);
                return entitiesInZone[i];
            }
        }
        print("Item " + itemName + " not found");
        return null;
    };

    InspectEntity.prototype = {
        
        preload: function(entityID) {
            this.entityID = entityID;
            print("PRELOAD INSPECT ENTITY");
            //get the owner ID from user data and compare to the mine
            //the update will be connected just for the owner
            var ownerObj = getEntityCustomData('ownerKey', this.entityID, null);
            if (ownerObj.ownerID === MyAvatar.sessionUUID) {
                rightController = new MyController(RIGHT_HAND);     //rightController and leftController are two objects
                leftController = new MyController(LEFT_HAND);
                workingHand = rightController;
                inspectingMyItem = true;
                inspectRadius = (Entities.getEntityProperties(_this.entityID).dimensions.x) / 2 + COMFORT_ARM_LENGTH;
                Script.update.connect(update);
            }
        },
        
        //Put the item which calls this method in inspect mode if it belongs to the owner of the inspect zone
        doSomething: function (entityID, dataArray) {
            var data = JSON.parse(dataArray[0]);
            var itemOwnerObj = getEntityCustomData('ownerKey', data.id, null);
            
            var inspectOwnerObj = getEntityCustomData('ownerKey', this.entityID, null);
            
            if (inspectOwnerObj == null) {
                Entities.deleteEntity(data.id);
            }
            
            if (itemOwnerObj.ownerID === inspectOwnerObj.ownerID) {
                //setup the things for inspecting the item
                inspecting = true;
                inspectedEntityID = data.id;        //store the ID of the inspected entity
                setEntityCustomData('statusKey', data.id, {
                    status: IN_INSPECT_STATUS
                });
                _this.createInspectUI();
                itemOriginalDimensions = Entities.getEntityProperties(inspectedEntityID).dimensions;
                
                // find separator and camera and hide
                separator = findItemByName(inspectedEntityID, SEPARATOR);
                Entities.editEntity(separator, { locked: false });
                Entities.editEntity(separator, { visible: false });
                print("Got here! Entity to hide: " + separator);
                cameraReview = findItemByName(inspectedEntityID, CAMERA_REVIEW);
                Entities.editEntity(cameraReview, { locked: false });
                Entities.editEntity(cameraReview, { visible: false });
                print("Got here! Entity to hide: " + cameraReview);
            } else {
                Entities.deleteEntity(data.id);
            }
        },
        
        //make the inspection entity stay at the proper position with respect to the avatar and camera positions
        positionRotationUpdate: function() {
            var newRotation;
            if (tryingOnAvatar) {
                newRotation = Vec3.sum(Quat.safeEulerAngles(MyAvatar.orientation), {x:0, y: 180, z: 0});        //neccessary to set properly the camera in entity mode when trying on avatar
                Entities.editEntity(_this.entityID, { rotation: Quat.fromVec3Degrees(newRotation) });
            } else {
                var newPosition = Vec3.sum(Camera.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), inspectRadius)); 
                Entities.editEntity(_this.entityID, { position: newPosition });
                newRotation = MyAvatar.orientation;
                Entities.editEntity(_this.entityID, { rotation: newRotation });
            }

            newPosition = Vec3.sum(newPosition, Vec3.multiply(Quat.getRight(newRotation), 0.34));

        },
        
        createInspectUI : function() {
            
            var infoObj = getEntityCustomData('infoKey', inspectedEntityID, null);
            var itemDescriptionString = null;
            var priceNumber = -1;
            var availabilityNumber = -1;
            var wearable = false;
            
            if(infoObj != null) {
                //var modelURLsLoop = infoObj.modelURLs; ??
                var rootURLString = infoObj.rootURL;
                for (var i = 0; i < infoObj.modelURLs.length; i++) {
                    modelURLsArray[i] = rootURLString + infoObj.modelURLs[i];
                    previewURLsArray[i] = rootURLString + infoObj.previewURLs[i];
                }
                
                itemDescriptionString = infoObj.description;
                priceNumber = infoObj.price;
                availabilityNumber = infoObj.availability;
                wearable = infoObj.wearable;
                infoObj = null;
            }
            
            //Looking for the item DB, this entity has in its userData the info of the customer reviews for the item
            var DBID = findItemDataBase(_this.entityID, Entities.getEntityProperties(inspectedEntityID).name);
            
            if (DBID != null) {
                infoObj = getEntityCustomData('infoKey', DBID, null);
                var scoreAverage = null;
                var reviewerName = null;
                
                if(infoObj != null) {
                    dbMatrix = infoObj.dbKey;
                    reviewsNumber = infoObj.dbKey.length;
                    //print("DB matrix is " + dbMatrix + " with element number: " + reviewsNumber);
                    var scoreSum = null;
                    
                    for (var i = 0; i < dbMatrix.length; i++) {
                        scoreSum += dbMatrix[i].score;
                    }
                    
                    if (dbMatrix.length) {
                        scoreAverage = Math.round(scoreSum / dbMatrix.length);
                        reviewerName = dbMatrix[reviewIndex].name;
                        
                         var message = {
                            command: "Show",
                            clip_url: dbMatrix[reviewIndex].clip_url
                        };
                        
                        Messages.sendMessage(AGENT_REVIEW_CHANNEL, JSON.stringify(message));
                        print("Show sent to agent");
                    } else {
                        //some default value if the DB is empty
                        scoreAverage = 0;
                        reviewerName = NO_REVIEWS_AVAILABLE;
                    }
                    
                }
                
                print ("Creating inspect UI");
                //set the main panel to follow the inspect entity
                mainPanel = new OverlayPanel({
                    anchorPositionBinding: { entity: _this.entityID },
                    anchorRotationBinding: { entity: _this.entityID },
                    isFacingAvatar: false
                });
                
                var offsetPositionY = 0.2;
                var offsetPositionX = -0.4;
                
                //these buttons are the 3 previews of the item
                for (var i = 0; i < previewURLsArray.length; i++) {
                    buttons[i] = new Image3DOverlay({
                        url: previewURLsArray[i],
                        dimensions: {
                            x: 0.15,
                            y: 0.15
                        },
                        isFacingAvatar: false,
                        alpha: 0.8,
                        ignoreRayIntersection: false,
                        offsetPosition: {
                            x: offsetPositionX,
                            y: offsetPositionY - (i * offsetPositionY),
                            z: 0
                        },
                        emissive: true,
                    });
                    
                    mainPanel.addChild(buttons[i]);
                }
                
                //aggregateScore is the average between those given by the reviewers
                var aggregateScore = new Image3DOverlay({
                    url: starConverter(scoreAverage),
                    dimensions: {
                        x: 0.25,
                        y: 0.25
                    },
                    isFacingAvatar: false,
                    alpha: 1,
                    ignoreRayIntersection: true,
                    offsetPosition: {
                        x: 0,
                        y: 0.27,
                        z: 0
                    },
                    emissive: true,
                });
                
                mainPanel.addChild(aggregateScore);
                
                //if any review is available create buttons to manage them
                if (dbMatrix.length) {
                    
                    playButton = new Image3DOverlay({
                        url: "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/Play.png",
                        dimensions: {
                            x: 0.08,
                            y: 0.08
                        },
                        isFacingAvatar: false,
                        alpha: 1,
                        ignoreRayIntersection: false,
                        offsetPosition: {
                            x: 0.42,
                            y: 0.27,
                            z: 0
                        },
                        emissive: true,
                    });
                    
                    mainPanel.addChild(playButton);
                    
                    
                    reviewerScore = new Image3DOverlay({
                        url: starConverter(dbMatrix[reviewIndex].score),
                        dimensions: {
                            x: 0.15,
                            y: 0.15
                        },
                        isFacingAvatar: false,
                        alpha: 1,
                        ignoreRayIntersection: true,
                        offsetPosition: {
                            x: 0.31,
                            y: 0.26,
                            z: 0
                        },
                        emissive: true,
                    });
                    
                    mainPanel.addChild(reviewerScore);
                    
                    nextButton = new Image3DOverlay({
                        url: "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/Next.png",
                        dimensions: {
                            x: 0.2,
                            y: 0.2
                        },
                        isFacingAvatar: false,
                        alpha: 1,
                        ignoreRayIntersection: false,
                        offsetPosition: {
                            x: 0.36,
                            y: 0.18,
                            z: 0
                        },
                        emissive: true,
                    });
                    
                    
                    mainPanel.addChild(nextButton);
                    
                }
                
                textReviewerName = new Text3DOverlay({
                        text: reviewerName,
                        isFacingAvatar: false,
                        alpha: 1.0,
                        ignoreRayIntersection: true,
                        offsetPosition: {
                            x: 0.23,
                            y: 0.31,
                            z: 0
                        },
                        dimensions: { x: 0, y: 0 },
                        backgroundColor: { red: 255, green: 255, blue: 255 },
                        color: { red: 0, green: 0, blue: 0 },
                        topMargin: 0.00625,
                        leftMargin: 0.00625,
                        bottomMargin: 0.1,
                        rightMargin: 0.00625,
                        lineHeight: 0.02,
                        alpha: 1,
                        backgroundAlpha: 0.3
                    });
                    
                mainPanel.addChild(textReviewerName);
                
                //if the item is wearable create a tryOnAvatarButton
                if (wearable) {
                    tryOnAvatarButton = new Image3DOverlay({
                        url: TRY_ON_ICON,
                        dimensions: {
                            x: 0.2,
                            y: 0.2
                        },
                        isFacingAvatar: false,
                        alpha: 1,
                        ignoreRayIntersection: false,
                        offsetPosition: {
                            x: 0.35,
                            y: -0.22,
                            z: 0
                        },
                        emissive: true,
                    });
                    
                    mainPanel.addChild(tryOnAvatarButton);
                }
                
                
                var textQuantityString = new Text3DOverlay({
                        text: "Quantity: ",
                        isFacingAvatar: false,
                        alpha: 1.0,
                        ignoreRayIntersection: true,
                        offsetPosition: {
                            x: 0.25,
                            y: -0.3,
                            z: 0
                        },
                        dimensions: { x: 0, y: 0 },
                        backgroundColor: { red: 255, green: 255, blue: 255 },
                        color: { red: 0, green: 0, blue: 0 },
                        topMargin: 0.00625,
                        leftMargin: 0.00625,
                        bottomMargin: 0.1,
                        rightMargin: 0.00625,
                        lineHeight: 0.02,
                        alpha: 1,
                        backgroundAlpha: 0.3
                    });
                    
                mainPanel.addChild(textQuantityString);
                
                var textQuantityNumber = new Text3DOverlay({
                        text: availabilityNumber,
                        isFacingAvatar: false,
                        alpha: 1.0,
                        ignoreRayIntersection: true,
                        offsetPosition: {
                            x: 0.28,
                            y: -0.32,
                            z: 0
                        },
                        dimensions: { x: 0, y: 0 },
                        backgroundColor: { red: 255, green: 255, blue: 255 },
                        color: { red: 0, green: 0, blue: 0 },
                        topMargin: 0.00625,
                        leftMargin: 0.00625,
                        bottomMargin: 0.1,
                        rightMargin: 0.00625,
                        lineHeight: 0.06,
                        alpha: 1,
                        backgroundAlpha: 0.3
                    });
                    
                mainPanel.addChild(textQuantityNumber);
                
                if (itemDescriptionString != null) {
                    var textDescription = new Text3DOverlay({
                        text: "Price: " + priceNumber + "\nAdditional information: \n" + itemDescriptionString,
                        isFacingAvatar: false,
                        alpha: 1.0,
                        ignoreRayIntersection: true,
                        offsetPosition: {
                            x: -0.2,
                            y: -0.3,
                            z: 0
                        },
                        dimensions: { x: 0, y: 0 },
                        backgroundColor: { red: 255, green: 255, blue: 255 },
                        color: { red: 0, green: 0, blue: 0 },
                        topMargin: 0.00625,
                        leftMargin: 0.00625,
                        bottomMargin: 0.1,
                        rightMargin: 0.00625,
                        lineHeight: 0.02,
                        alpha: 1,
                        backgroundAlpha: 0.3
                    });
                    
                    mainPanel.addChild(textDescription);
                }
                
            
                
                print ("GOT HERE: Descrition " + itemDescriptionString + " Availability " + availabilityNumber);
                
                isUIWorking = true;
            }
        },

        //Manage the collisions and tell to the item if it is into the this inspection area
        collisionWithEntity: function(myID, otherID, collisionInfo) {
            
            var itemObj = getEntityCustomData('itemKey', _this.entityID, null);
            if (itemObj != null) {
                if (itemObj.itemID == otherID) {        //verify that the inspect area is colliding with the actual item which created it
                    
                    var penetrationValue = Vec3.length(collisionInfo.penetration);
                    if (penetrationValue > PENETRATION_THRESHOLD && collidedItemID === null) {
                        collidedItemID = otherID;
                        print("Start collision with: " + Entities.getEntityProperties(collidedItemID).name);
                        Entities.callEntityMethod(otherID, 'changeOverlayColor', null);
                        
                    } else if (penetrationValue < PENETRATION_THRESHOLD && collidedItemID !== null) {
                        
                        print("End collision with: " + Entities.getEntityProperties(collidedItemID).name);
                        collidedItemID = null;
                        Entities.callEntityMethod(otherID, 'changeOverlayColor', null);
                    }
                }
            }
        },
        
        changeModel: function(index) {
            var entityProperties = Entities.getEntityProperties(inspectedEntityID);
            if (entityProperties.modelURL != modelURLsArray[index]) {
                Entities.editEntity(inspectedEntityID, { modelURL: modelURLsArray[index] });
            }
        },
        
        unload: function (entityID) {
            if(inspectingMyItem){
                print("UNLOAD INSPECT ENTITY");
                Script.update.disconnect(update);
                
                // clean UI
                Entities.deleteEntity(_this.entityID);
            }
        }
    };
    return new InspectEntity();
})