// shopCartEntityScript.js
//
//  This script makes the cart follow the avatar who picks it and manage interations with items (store and delete them) and with cash register (send the item prices)

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
    
    var COMFORT_ARM_LENGTH = 0.5;
    var CART_REGISTER_CHANNEL = "Hifi-vrShop-Register";
    var PENETRATION_THRESHOLD = 0.2;
    
    var _this;
    var cartIsMine = false;
    var originalY = 0;
    var itemsID = [];
    var scaleFactor = 0.7; //TODO: The scale factor will dipend on the number of items in the cart. We would resize even the items already present.
    var cartTargetPosition;
    var singlePrices = [];
    var singlePriceTagsAreShowing = false;
    var collidedItemID = null;

    // this is the "constructor" for the entity as a JS object we don't do much here, but we do want to remember
    // our this object, so we can access it in cases where we're called without a this (like in the case of various global signals)
    ShopCart = function() { 
         _this = this;
    };
    
    function update(deltaTime) {
        _this.followAvatar();
        
        if (Controller.getValue(Controller.Standard.RightPrimaryThumb)) {
            _this.resetCart();
            _this.computeAndSendTotalPrice();
            
        }
    };
    
    function receivingMessage(channel, message, senderID) {
        if (senderID === MyAvatar.sessionUUID && channel == CART_REGISTER_CHANNEL) {
            var messageObj = JSON.parse(message);
            if (messageObj.senderEntity != _this.entityID) {
                print("--------------- cart received message");
                //Receiving this message means that the register wants the total price
                _this.computeAndSendTotalPrice();
            }
        }
        
    };

    ShopCart.prototype = {

        // preload() will be called when the entity has become visible (or known) to the interface
        // it gives us a chance to set our local JavaScript object up. In this case it means:
        //   * remembering our entityID, so we can access it in cases where we're called without an entityID
        //   * connecting to the update signal so we can check our grabbed state
        preload: function(entityID) {
            this.entityID = entityID;
            //get the owner ID from user data and compare to the mine
            //so the update will be connected just for the owner
            var ownerObj = getEntityCustomData('ownerKey', this.entityID, null);
            if (ownerObj.ownerID === MyAvatar.sessionUUID) {
                cartIsMine = true;
                cartTargetPosition = Entities.getEntityProperties(_this.entityID).position; //useful if the entity script is assigned manually
                Script.update.connect(update);
                Messages.subscribe(CART_REGISTER_CHANNEL);
                Messages.messageReceived.connect(receivingMessage);
            }
        },
        
        //update cart's target position. It will be at the right of the avatar as long as he moves
        followAvatar: function() {
            if (Vec3.length(MyAvatar.getVelocity()) > 0.1) {
                var radius = (Entities.getEntityProperties(_this.entityID).dimensions.x) / 2 + COMFORT_ARM_LENGTH;
                var properY = MyAvatar.position.y + ((MyAvatar.getHeadPosition().y - MyAvatar.position.y) / 2);
                var targetPositionPrecomputing = {x: MyAvatar.position.x, y: properY, z: MyAvatar.position.z};
                cartTargetPosition = Vec3.sum(targetPositionPrecomputing, Vec3.multiply(Quat.getRight(MyAvatar.orientation), radius));
            }
            
            var cartPosition = Entities.getEntityProperties(_this.entityID).position;
            var positionDifference = Vec3.subtract(cartTargetPosition, cartPosition);
            if (Vec3.length(positionDifference) > 0.1) {
                //give to the cart the proper velocity and make it ignore for collision
                Entities.editEntity(_this.entityID, { velocity: positionDifference });
                Entities.editEntity(_this.entityID, { ignoreForCollisions: true });
                if (collidedItemID != null) {
                    Entities.callEntityMethod(collidedItemID, 'setCartOverlayNotVisible', null);
                    collidedItemID = null;
                }
            } else if (Vec3.length(positionDifference) > 0.01) {
                //give to the cart the proper velocity and make it NOT ignore for collision
                Entities.editEntity(_this.entityID, { velocity: positionDifference });
                Entities.editEntity(_this.entityID, { ignoreForCollisions: false });
            } else if (Vec3.length(positionDifference) > 0) {
                //set the position to be at the right of MyAvatar and make it NOT ignore for collision
                Entities.editEntity(_this.entityID, { position: cartTargetPosition });
                positionDifference = Vec3.subtract(cartTargetPosition, cartPosition);
                Entities.editEntity(_this.entityID, { velocity: positionDifference });
                Entities.editEntity(_this.entityID, { ignoreForCollisions: false });
            }

        },
        
        // delete all items stored into the cart
        resetCart: function () {
            
            //print("RESET CART - USER DATA: " + Entities.getEntityProperties(_this.entityID).userData);
            if (itemsID.length != 0) {
                if (singlePriceTagsAreShowing) {
                    _this.singlePriceOff();
                }
                for (var i=0; i < itemsID.length; i++) {
                    Entities.deleteEntity(itemsID[i]);
                }
                
                // Clear the userData field for the cart
                Entities.editEntity(this.entityID, { userData: ""});
                
                setEntityCustomData('ownerKey', this.entityID, {
                    ownerID: MyAvatar.sessionUUID
                });
                
                setEntityCustomData('grabbableKey', this.entityID, {
                    grabbable: false
                });
                
                itemsID = [];
            }
        },
        
        //delete the item pointed by dataArray (data.id) from the cart because it's been grabbed from there
        refreshCartContent: function (entityID, dataArray) {
            var data = JSON.parse(dataArray[0]);
            
            for (var i=0; i < itemsID.length; i++) {
                if(itemsID[i] == data.id) {
                    itemsID.splice(i, 1);
                    //if the price tags are showing we have to remove also the proper tag
                    if (singlePriceTagsAreShowing) {
                        singlePrices[i].destroy();
                        singlePrices.splice(i, 1);
                    }
                }
            }
            
            _this.computeAndSendTotalPrice();
        },
        
        //show the prices on each item into the cart
        singlePriceOn: function () {
            //create an array of text3D which follows the structure of the itemsID array. Each text3D is like the 'Store the item!' one
            var i = 0;
            itemsID.forEach( function(itemID) {
                singlePrices[i] = new OverlayPanel({
                    anchorPositionBinding: { entity: itemID },
                    offsetPosition: { x: 0, y: 0.15, z: 0 },
                    isFacingAvatar: true,
                    
                });
                
                var textPrice = new Text3DOverlay({
                    text: "" + getEntityCustomData('infoKey', itemID, null).price + " $",
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
                    visible: true
                });
                
                singlePrices[i].addChild(textPrice);
                i++;
            });
            
            singlePriceTagsAreShowing = true;
        },
        
        singlePriceOff: function () {
            //destroy or make invisible the text3D, or both
            singlePrices.forEach(function(panel) {
                panel.destroy();
            });
            singlePrices = [];
            singlePriceTagsAreShowing = false;
        },
        
        //Send to the register the total price for all the items in the cart
        computeAndSendTotalPrice: function () {
            var totalPrice = 0;
            itemsID.forEach( function(itemID) {
                var infoObj = getEntityCustomData('infoKey', itemID, null);
                if(infoObj != null) {
                    totalPrice += infoObj.price;
                }
            });
            
            var messageObj = {senderEntity: _this.entityID, totalPrice: totalPrice};
            Messages.sendMessage(CART_REGISTER_CHANNEL, JSON.stringify(messageObj));
        },
        
        //dataArray stores the ID of the item which has to be stored into the cart
        //this entity method is invoked by shopItemEntityScript.js
        doSomething: function (entityID, dataArray) {
            collidedItemID = null;
            var data = JSON.parse(dataArray[0]);
            var itemOwnerObj = getEntityCustomData('ownerKey', data.id, null);
            
            var cartOwnerObj = getEntityCustomData('ownerKey', this.entityID, null);
            
            if (cartOwnerObj == null) {
                //print("The cart doesn't have a owner.");
                Entities.deleteEntity(data.id);
            }
            
            if (itemOwnerObj.ownerID === cartOwnerObj.ownerID) {
                // TODO if itemsQuantity == fullCart resize all the items present in the cart and change the scaleFactor for this and next insert

                print("Going to put item in the cart!");
                var itemsQuantity = itemsID.length;
                
                itemsID[itemsQuantity] = data.id;
                
                var oldDimension = Entities.getEntityProperties(data.id).dimensions;
                Entities.editEntity(data.id, { dimensions: Vec3.multiply(oldDimension, scaleFactor) });
                Entities.editEntity(data.id, { velocity: {x: 0.0, y: 0.0, z: 0.0} });
                // parent item to the cart
                Entities.editEntity(data.id, { parentID: this.entityID });
                
                itemsQuantity = itemsID.length;
                
                setEntityCustomData('statusKey', data.id, {
                    status: "inCart"
                });
                
                _this.computeAndSendTotalPrice();
                
                //if the single price tags are showing we have to put a tag also in the new item
                if (singlePriceTagsAreShowing) {
                    singlePrices[itemsQuantity-1] = new OverlayPanel({
                        anchorPositionBinding: { entity: data.id },
                        offsetPosition: { x: 0, y: 0.15, z: 0 },
                        isFacingAvatar: true,
                    
                    });
                
                    var textPrice = new Text3DOverlay({
                        text: "" + getEntityCustomData('infoKey', data.id, null).price + " $",
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
                        visible: true
                    });
                
                    singlePrices[itemsQuantity-1].addChild(textPrice);
                }
            } else {
                print("Not your cart!");
                Entities.deleteEntity(data.id);
            }
        },
        
        //detect when the item enters or leave the cart while it's grabbed
        collisionWithEntity: function(myID, otherID, collisionInfo) {
            var penetrationValue = Vec3.length(collisionInfo.penetration);
            
            var cartOwnerObj = getEntityCustomData('ownerKey', myID, null);
            var itemOwnerObj = getEntityCustomData('ownerKey', otherID, null);
            
            if (penetrationValue > PENETRATION_THRESHOLD && collidedItemID === null) {
                if (itemOwnerObj != null && itemOwnerObj.ownerID === cartOwnerObj.ownerID) {
                    Entities.callEntityMethod(otherID, 'setCartOverlayVisible', null);
                    collidedItemID = otherID;
                }
            } else if (penetrationValue < PENETRATION_THRESHOLD && collidedItemID !== null) {
                if (itemOwnerObj != null && itemOwnerObj.ownerID === cartOwnerObj.ownerID) {
                    Entities.callEntityMethod(otherID, 'setCartOverlayNotVisible', null);
                    collidedItemID = null;
                }
            }
        },
        
        unload: function (entityID) {
            print("UNLOAD CART");
            if(cartIsMine){
                Script.update.disconnect(update);
                _this.resetCart();  //useful if the script is reloaded manually
                //Entities.deleteEntity(_this.entityID);        //comment for manual reload
                Messages.unsubscribe(CART_REGISTER_CHANNEL);
                Messages.messageReceived.disconnect(receivingMessage);
            }
        }
    };

    // entity scripts always need to return a newly constructed object of our type
    return new ShopCart();
})