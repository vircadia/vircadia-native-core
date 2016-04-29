// shopItemEntityScript.js
//
//  This script makes the shop items react properly to the grab (see shopItemGrab.js) and gives it the capability to interact with the shop environment (inspection entity and cart)
//  The first time the item is grabbed a copy of itself is created on the shelf.
//  If the item isn't held by the user it can be inInspect or inCart, otherwise it's destroyed
//  The start and release grab methods handle the creation of the item copy, UI, inspection entity
//  If the item is released into a valid zone, the doSomething() method of that zone is called and the item sends its ID

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function() {
    var utilitiesScript = Script.resolvePath("../../libraries/utils.js");
    var overlayManagerScript = Script.resolvePath("../../libraries/overlayManager.js");
    var inspectEntityScript = Script.resolvePath("../inspect/shopInspectEntityScript.js");
    
    Script.include(utilitiesScript);
    Script.include(overlayManagerScript);

    
    var RED_IMAGE_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/inspRED.png";
    var GREEN_IMAGE_URL = "https://dl.dropboxusercontent.com/u/14127429/FBX/VRshop/inspGREEN.png";
    var MIN_DIMENSION_THRESHOLD = null;
    var MAX_DIMENSION_THRESHOLD = null;
    var PENETRATION_THRESHOLD = 0.2;
    var MAPPING_NAME = "controllerMapping_Inspection";
    var SHOPPING_CART_NAME = "Shopping cart";

    var _this;
    var onShelf = true;
    var inspecting = false;
    var inCart = false;
    var overlayInspectRed = true;
    var zoneID = null;
    var newPosition;
    var originalDimensions = null;
    var deltaLX = 0;
    var deltaLY = 0;
    var deltaRX = 0;
    var deltaRY = 0;
    var inspectingEntity = null;
    var inspectPanel = null;
    var background = null;
    var textCart = null;
    var cartID = null;
    
    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    ItemEntity = function() { 
         _this = this;
    };
    
    function update(deltaTime) {
        if(inspecting){
            _this.orientationPositionUpdate();
        }
    };
    

    ItemEntity.prototype = {

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            this.entityID = entityID;
            print("PRELOAD: " + Entities.getEntityProperties(this.entityID).name + " " + entityID + " at:");
            Vec3.print(Entities.getEntityProperties(this.entityID).name, Entities.getEntityProperties(this.entityID).position);
            
            Script.update.connect(update);
    
            MIN_DIMENSION_THRESHOLD = Vec3.length(Entities.getEntityProperties(this.entityID).dimensions)/2;
            MAX_DIMENSION_THRESHOLD = Vec3.length(Entities.getEntityProperties(this.entityID).dimensions)*2;
            
        },
        
        //create a rectangle red or green do guide the user in putting the item in the inspection area
        createInspectOverlay: function (entityBindID) {
            inspectPanel = new OverlayPanel({
                anchorPositionBinding: { entity: entityBindID },
                anchorRotationBinding: { entity: entityBindID },
                isFacingAvatar: false
            });
            
            background = new Image3DOverlay({
                url: RED_IMAGE_URL,
                dimensions: {
                    x: 0.6,
                    y: 0.6,
                },
                isFacingAvatar: false,
                alpha: 1,
                ignoreRayIntersection: false,
                offsetPosition: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                emissive: true,
            });
            
            inspectPanel.addChild(background);
        },
        
        //add an overlay on the item grabbed by the customer to give him the feedback if the item is inside the bounding box of the cart
        createCartOverlay: function (entityBindID) {
            cartPanel = new OverlayPanel({
                anchorPositionBinding: { entity: entityBindID },
                offsetPosition: { x: 0, y: 0.2, z: 0.1 },
                isFacingAvatar: true,
                
            });
            var entityProperties = Entities.getEntityProperties(entityBindID);
            var userDataObj = JSON.parse(entityProperties.userData);
            var availabilityNumber = userDataObj.infoKey.availability;
            
            textCart = new Text3DOverlay({
                    text: availabilityNumber > 0 ? "Store the item!" : "Not available!",
                    isFacingAvatar: false,
                    alpha: 1.0,
                    ignoreRayIntersection: true,
                    dimensions: { x: 0, y: 0 },
                    backgroundColor: { red: 255, green: 255, blue: 255 },
                    color: { red: 0, green: 0, blue: 0 },
                    topMargin: 0.00625,
                    leftMargin: 0.00625,
                    bottomMargin: 0.1,
                    rightMargin: 0.00625,
                    lineHeight: 0.02,
                    alpha: 1,
                    backgroundAlpha: 0.3,
                    visible: false
                });
            
            cartPanel.addChild(textCart);
            
        },
        
        
        
        setCartOverlayVisible: function () {
            textCart.visible = true;
        },
        
        setCartOverlayNotVisible: function () {
            textCart.visible = false;
        },
        
        changeOverlayColor: function () {
            if (overlayInspectRed) {
                //print ("Change color of overlay to green");
                overlayInspectRed = false;
                background.url = GREEN_IMAGE_URL;
            } else {
                //print ("Change color of overlay to red");
                overlayInspectRed = true;
                background.url = RED_IMAGE_URL;
            }
        },
        
        //What is done by this methos changes acoordingly to the previous state of the item - onShelf , inCart, inInspect
        startNearGrab: function () {
            
            //we have to distinguish if the user who is grabbing has a cart or not
            //if he does it is a buyer, otherwise he probably want to do a review
            var thisItemPosition = Entities.getEntityProperties(_this.entityID).position;
            
            // if onShelf is true, this is the first grab
            if (onShelf === true) {
                
                var foundEntities = Entities.findEntities(thisItemPosition, 5);
                foundEntities.forEach( function (foundEntityID) {
                    var entityName = Entities.getEntityProperties(foundEntityID).name;
                    if (entityName == SHOPPING_CART_NAME) {
                        var cartOwnerID = getEntityCustomData('ownerKey', foundEntityID, null).ownerID;
                        if (cartOwnerID == MyAvatar.sessionUUID) {
                            cartID = foundEntityID;
                        }
                    }
                });
                
                // Create a copy of this entity if it is the first grab
                print("creating a copy of the grabbed dentity");
                var entityProperties = Entities.getEntityProperties(_this.entityID);
                
                var entityOnShelf = Entities.addEntity({
                    type: entityProperties.type,
                    name: entityProperties.name,
                    position: thisItemPosition,
                    dimensions: entityProperties.dimensions,
                    rotation: entityProperties.rotation,
                    collisionsWillMove: false,
                    ignoreForCollisions: true,
                    modelURL: entityProperties.modelURL,
                    shapeType: entityProperties.shapeType,
                    originalTextures: entityProperties.originalTextures,
                    script: entityProperties.script,
                    userData: entityProperties.userData
                });
                
                var tempUserDataObj = JSON.parse(entityProperties.userData);
                var availabilityNumber = tempUserDataObj.infoKey.availability;
                
                if (availabilityNumber > 0 && cartID) {
                    tempUserDataObj.infoKey.availability  = tempUserDataObj.infoKey.availability - 1;
                    setEntityCustomData('infoKey', entityOnShelf, tempUserDataObj.infoKey);
                }
                
                setEntityCustomData('statusKey', _this.entityID, {
                    status: "inHand"
                });
                
                onShelf = false;
                setEntityCustomData('ownerKey', _this.entityID, {
                    ownerID: MyAvatar.sessionUUID
                });
                originalDimensions = entityProperties.dimensions;

            }
            
            //if cartID is defined the user is a buyer
            if (cartID) {
                // Everytime we grab, we create the inspectEntity and the inspectAreaOverlay in front of the avatar
                if(!inspecting) {
                    inspectingEntity = Entities.addEntity({
                        type: "Box",
                        name: "inspectionEntity",
                        dimensions: {x: 0.5, y: 0.5, z: 0.5},
                        collisionsWillMove: false,
                        ignoreForCollisions: false,
                        visible: false,
                        script: inspectEntityScript,
                        userData: JSON.stringify({
                            ownerKey: {
                                ownerID: MyAvatar.sessionUUID
                            },
                            itemKey: {
                                itemID: _this.entityID
                            },
                            grabbableKey: {
                                grabbable: false
                            }
                        })
                    });
                }
                
                Entities.editEntity(_this.entityID, { ignoreForCollisions: false });
                _this.createInspectOverlay(inspectingEntity);
                _this.createCartOverlay(_this.entityID);
                
                if (inspecting === true) {
                    inspecting = false;
                    Entities.editEntity(_this.entityID, { dimensions: originalDimensions });
                    Controller.disableMapping(MAPPING_NAME);
                    setEntityCustomData('statusKey', _this.entityID, {
                        status: "inHand"
                    });
                } else if (inCart === true) {
                    inCart = false;
                    Entities.editEntity(_this.entityID, { dimensions: originalDimensions });
                    setEntityCustomData('statusKey', _this.entityID, {
                        status: "inHand"
                    });
                    var dataJSON = {
                        id: _this.entityID
                    };
                    var dataArray = [JSON.stringify(dataJSON)];
                    Entities.callEntityMethod(zoneID, 'refreshCartContent', dataArray);
                }
            }
        },
        
        continueNearGrab: function () {
        },

        //Every time the item is released I have to check what's the role of the user and where the release happens
        releaseGrab: function () {
            
            //if cart ID is not defined destroy the item whatever the zone is because the user is a reviewer
            if (!cartID) {
                Entities.deleteEntity(this.entityID);
                return;
            }
            
            Entities.editEntity(_this.entityID, { ignoreForCollisions: true });
            // Destroy overlay
            inspectPanel.destroy();
            cartPanel.destroy();
            inspectPanel = cartPanel = null;
            
            if (zoneID !== null) {
                var dataJSON = {
                    id: _this.entityID
                };
                var dataArray = [JSON.stringify(dataJSON)];
                Entities.callEntityMethod(zoneID, 'doSomething', dataArray);
                
                var statusObj = getEntityCustomData('statusKey', _this.entityID, null);
                
                //There are two known zones where the relase can happen: inspecting area and cart
                if (statusObj.status == "inInspect") {
                    inspecting = true;
                    
                    var mapping = Controller.newMapping(MAPPING_NAME);
                    mapping.from(Controller.Standard.LX).to(function (value) {
                        deltaLX = value;
                    });
                    mapping.from(Controller.Standard.LY).to(function (value) {
                        deltaLY = value;
                    });
                    mapping.from(Controller.Standard.RX).to(function (value) {
                        deltaRX = value;
                    });
                    mapping.from(Controller.Standard.RY).to(function (value) {
                        deltaRY = value;
                    });
                    Controller.enableMapping(MAPPING_NAME);
                } else if (statusObj.status == "inCart") {
                    Entities.deleteEntity(inspectingEntity);
                    inspectingEntity = null;
                    
                    var entityProperties = Entities.getEntityProperties(this.entityID);
                    var userDataObj = JSON.parse(entityProperties.userData);
                    var availabilityNumber = userDataObj.infoKey.availability;
                    //if the item is no more available, destroy it
                    if (availabilityNumber == 0) {
                        Entities.deleteEntity(this.entityID);
                    }
                    print("inCart is TRUE");
                    inCart = true;
                } else { // any other zone
                    //print("------------zoneID is something");
                    Entities.deleteEntity(inspectingEntity);
                    inspectingEntity = null;
                }
                
            } else { // ZoneID is null, released somewhere that is not a zone.
            //print("------------zoneID is null");
                Entities.deleteEntity(inspectingEntity);
                inspectingEntity = null;
                Entities.deleteEntity(this.entityID);
            }
        
        },

        //Analysing the collisions we can define the value of zoneID
        collisionWithEntity: function(myID, otherID, collisionInfo) {
            var penetrationValue = Vec3.length(collisionInfo.penetration);
            if (penetrationValue > PENETRATION_THRESHOLD && zoneID === null) {
                zoneID = otherID;
                print("Item IN: " + Entities.getEntityProperties(zoneID).name);
            } else if (penetrationValue < PENETRATION_THRESHOLD && zoneID !== null && otherID == zoneID) {
                print("Item OUT: " + Entities.getEntityProperties(zoneID).name);
                zoneID = null;
            }
        },
        
        //this method handles the rotation and scale of the item while in inspect and also guarantees to keep the item in the proper position. It's done every update
        orientationPositionUpdate: function() {
            //position
            var inspectingEntityPosition = Entities.getEntityProperties(inspectingEntity).position;      //at this time inspectingEntity is a valid entity
            var inspectingEntityRotation = Entities.getEntityProperties(inspectingEntity).rotation;
            newPosition = Vec3.sum(inspectingEntityPosition, Vec3.multiply(Quat.getFront(inspectingEntityRotation), -0.2));   //put the item near to the face of the user
            Entities.editEntity(_this.entityID, { position: newPosition });
            //orientation
            var newRotation = Quat.multiply(Entities.getEntityProperties(_this.entityID).rotation, Quat.fromPitchYawRollDegrees(deltaRY*10, deltaRX*10, 0))
            Entities.editEntity(_this.entityID, { rotation: newRotation });
            //zoom
            var oldDimension = Entities.getEntityProperties(_this.entityID).dimensions;
            var scaleFactor = (deltaLY * 0.1) + 1;
            if (!((Vec3.length(oldDimension) < MIN_DIMENSION_THRESHOLD && scaleFactor < 1) || (Vec3.length(oldDimension) > MAX_DIMENSION_THRESHOLD && scaleFactor > 1))) {
                var newDimension = Vec3.multiply(oldDimension, scaleFactor);
                Entities.editEntity(_this.entityID, { dimensions: newDimension });
            }
        },
        
        unload: function (entityID) {
            Script.update.disconnect(update);
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new ItemEntity();
})