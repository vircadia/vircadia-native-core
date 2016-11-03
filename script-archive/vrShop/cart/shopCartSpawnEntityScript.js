// shopCartSpawnEntityScript.js
//
//  If an avatar doesn't own a cart and enters the zone, a cart is added.
//  Otherwise if it already has a cart, this will be destroyed

//  Created by Alessandro Signa and Edgar Pironti on 01/13/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


(function () {
    var CART_MASTER_NAME = "ShopCartZero";
    var CART_SCRIPT_URL = Script.resolvePath("shopCartEntityScript.js");
    var SHOP_GRAB_SCRIPT_URL = Script.resolvePath("../item/shopItemGrab.js");
    var _this;
    var isOwningACart = false;
    var cartMasterID = null;
    var myCartID = null;


    function SpawnCartZone() {
        _this = this;
        return;
    };



    SpawnCartZone.prototype = {

        preload: function (entityID) {
            this.entityID = entityID;
            // Look for the ShopCartZero. Every cart created by this script is a copy of it
            var ids = Entities.findEntities(Entities.getEntityProperties(this.entityID).position, 50);
            ids.forEach(function(id) {
                var properties = Entities.getEntityProperties(id);
                if (properties.name == CART_MASTER_NAME) {
                    cartMasterID = id;
                    print("Master Cart found");
                    Entities.editEntity(_this.entityID, { collisionMask: "static,dynamic,otherAvatar" });
                    
                    return;
                }
            });
        },

        enterEntity: function (entityID) {
            print("entering in the spawn cart area");
            
            if (myCartID) {
                Entities.callEntityMethod(myCartID, "resetCart");
                Entities.deleteEntity (myCartID);
                myCartID = null;
            } else {
                var entityProperties = Entities.getEntityProperties(cartMasterID);
                myCartID = Entities.addEntity({
                    type: entityProperties.type,
                    name: "Shopping cart",
                    ignoreForCollisions: false,
                    collisionsWillMove: false,
                    position: entityProperties.position,
                    dimensions: entityProperties.dimensions,
                    modelURL: entityProperties.modelURL,
                    shapeType: entityProperties.shapeType,
                    originalTextures: entityProperties.originalTextures,
                    script: CART_SCRIPT_URL,
                    userData: JSON.stringify({
                        ownerKey: {
                            ownerID: MyAvatar.sessionUUID
                        },
                        grabbableKey: {
                            grabbable: false
                        }
                    })
                });
            }
        },

        leaveEntity: function (entityID) {
            
        },

        unload: function (entityID) {
           
        }
    }

    return new SpawnCartZone();
});