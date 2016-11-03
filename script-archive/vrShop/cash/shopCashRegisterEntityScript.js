// shopCashRegisterEntityScript.js
//
//  The register manages the total price overlay and the payment through collision with credit card

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

(function () {
    var overlayManagerScript = Script.resolvePath("../../libraries/overlayManager.js");
    Script.include(overlayManagerScript);
    
    var SHOPPING_CART_NAME = "Shopping cart";
    var CART_REGISTER_CHANNEL = "Hifi-vrShop-Register";
    var CREDIT_CARD_NAME = "CreditCard";
    
    var _this;
    var cartID = null;
    var registerPanel = null;
    var priceText = null;
    var payingAvatarID = null;
    var totalPrice = 0;

    function CashRegister() {
        _this = this;
        return;
    };
    
    function receivingMessage(channel, message, senderID) {
        if (senderID === MyAvatar.sessionUUID && channel == CART_REGISTER_CHANNEL) {
            var messageObj = JSON.parse(message);
            if (messageObj.senderEntity != _this.entityID) {
                //This message means that the cart sent the total price: create or update the Overlay on the register
                var price = messageObj.totalPrice.toFixed(2);
                _this.cashRegisterOverlayOn("" + price + " $");
                totalPrice = messageObj.totalPrice;
            }
        }
    };

    CashRegister.prototype = {

        preload: function (entityID) {
            this.entityID = entityID;
        },
        
        //This method is called by the cashZone when an avatar comes in it
        //It has to find the cart belonging to that avatar and ask it the total price of the items
        cashRegisterOn: function() {
            print("cashRegister ON");
            Messages.subscribe(CART_REGISTER_CHANNEL);
            Messages.messageReceived.connect(receivingMessage);
            var cashRegisterPosition = Entities.getEntityProperties(_this.entityID).position;
            var foundEntities = Entities.findEntities(cashRegisterPosition, 50);
            foundEntities.forEach( function (foundEntityID) {
                var entityName = Entities.getEntityProperties(foundEntityID).name;
                if (entityName == SHOPPING_CART_NAME) {
                    var cartOwnerID = getEntityCustomData('ownerKey', foundEntityID, null).ownerID;
                    if (cartOwnerID == MyAvatar.sessionUUID) {
                        cartID = foundEntityID;
                        
                    }
                }
            });
            if (cartID != null) {
                payingAvatarID = MyAvatar.sessionUUID;
                Messages.sendMessage(CART_REGISTER_CHANNEL, JSON.stringify({senderEntity: _this.entityID}));    //with this message the cart know that it has to compute and send back the total price of the items
                Entities.callEntityMethod(cartID, 'singlePriceOn', null);
                //sometimes the cart is unable to receive the message. I think it's a message mixer problem
            } else {
                payingAvatarID = null;
                // Show anyway the overlay with the price 0$
                _this.cashRegisterOverlayOn("0 $");
            }
        },
        
        cashRegisterOff: function() {
            print("cashRegister OFF");
            Messages.unsubscribe(CART_REGISTER_CHANNEL);
            Messages.messageReceived.disconnect(receivingMessage);
            priceText.visible = false;
            if (cartID != null) {
                Entities.callEntityMethod(cartID, 'singlePriceOff', null);
            }
        },
        
        cashRegisterOverlayOn: function (string) {
            print("cashRegister OVERLAY ON");
            var stringOffset = string.length * 0.018;
            if (priceText == null) {
                
                registerPanel = new OverlayPanel({
                    anchorPositionBinding: { entity: _this.entityID },
                    offsetPosition: { x: 0, y: 0.21, z: -0.14 },
                    isFacingAvatar: false,
                    
                });
                
                priceText = new Text3DOverlay({
                        text: string,
                        isFacingAvatar: false,
                        ignoreRayIntersection: true,
                        dimensions: { x: 0, y: 0 },
                        offsetPosition: {
                            x: -stringOffset,
                            y: 0,
                            z: 0
                        },
                        backgroundColor: { red: 255, green: 255, blue: 255 },
                        color: { red: 0, green: 255, blue: 0 },
                        topMargin: 0.00625,
                        leftMargin: 0.00625,
                        bottomMargin: 0.1,
                        rightMargin: 0.00625,
                        lineHeight: 0.06,
                        alpha: 1,
                        backgroundAlpha: 0.3,
                        visible: true
                    });
                
                registerPanel.addChild(priceText);
            } else {
                priceText.text = string;
                priceText.visible = true;
                priceText.offsetPosition = {
                            x: -stringOffset,
                            y: 0,
                            z: 0
                        };
            }
        },
        
        //Manage the collision with credit card
        collisionWithEntity: function (myID, otherID, collisionInfo) {
            var entityName = Entities.getEntityProperties(otherID).name;
            var entityOwnerID = getEntityCustomData('ownerKey', otherID, null).ownerID;
            if (entityName == CREDIT_CARD_NAME && entityOwnerID == payingAvatarID) {
                //The register collided with the right credit card - CHECKOUT
                Entities.deleteEntity(otherID);
                Entities.callEntityMethod(cartID, 'resetCart', null);
                _this.cashRegisterOverlayOn("THANK YOU!");
                _this.clean();
            }
        },
        
        //clean all the variable related to the cart
        clean: function () {
            cartID = null;
            payingAvatarID = null;
            totalPrice = 0;
        },

        unload: function (entityID) {
            _this.clean();
            if (registerPanel != null) {
                registerPanel.destroy();
            }
            registerPanel = priceText = null;
        }
    }

    return new CashRegister();
});