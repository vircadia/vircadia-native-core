(function () {

    var _entityID;
    var gunID;

    function generateQuatFromDegreesViaRadians(rotxdeg,rotydeg,rotzdeg) {
        var rotxrad = (rotxdeg/180)*Math.PI;
        var rotyrad = (rotydeg/180)*Math.PI;
        var rotzrad = (rotzdeg/180)*Math.PI;          
        var newRotation = Quat.fromPitchYawRollRadians(rotxrad,rotyrad,rotzrad); 
        return newRotation;
    }

    function equipGun() {
        var RIGHT_HAND_INDEX = MyAvatar.getJointIndex("RightHand");
        var localRot = generateQuatFromDegreesViaRadians(71.87 , 92 , -16.92);
        var gunURL = "https://bas-skyspace.ams3.digitaloceanspaces.com/MurderGame/gun.fbx?" + Date.now();

        gunID = Entities.addEntity( {
            type: "Model",
            name: "MurderGameGun",
            modelURL: gunURL,
            parentID: MyAvatar.sessionUUID,
            parentJointIndex: RIGHT_HAND_INDEX,
            localPosition: { x: 0.0179, y: 0.1467, z: 0.0305 },
            localRotation: localRot,
            localDimensions: { x: 0.0323, y: 0.1487, z: 0.2328 },                            
            color: { red: 200, green: 0, blue: 20 }, 
            collisionless: true,               
            dynamic: false,                
            lifetime: -1,
            userData: "{ \"grabbableKey\": { \"grabbable\": false, \"triggerable\": false}}" 
        },"avatar");

        var messageToSend = {
            'command': 'register-with-entity-menu',
            'entityID': gunID,
            'menuItems': ['Unequip']
        };
        
        Messages.sendLocalMessage('nyx-ui', JSON.stringify(messageToSend));
    }

    function onMessageReceived(channel, message, senderID, localOnly) {
        if (channel === 'nyx-ui' && MyAvatar.sessionUUID === senderID) {
            messageData = JSON.parse(message);

            if (messageData.menuItem === 'Equip' && messageData.entityID === _entityID) {
                equipGun();
            }
            
            if (messageData.menuItem === 'Unequip' && messageData.entityID === gunID) {
                Entities.deleteEntity(gunID);
            }
        }
    }

    this.preload = function (entityID) {
        _entityID = entityID;
        Messages.subscribe('nyx-ui');
        Messages.messageReceived.connect(onMessageReceived);
        
        var messageToSend = {
            'command': 'register-with-entity-menu',
            'entityID': entityID,
            'menuItems': ['Equip']
        };
        
        Messages.sendLocalMessage('nyx-ui', JSON.stringify(messageToSend));
    };

    this.unload = function () {
        Messages.unsubscribe('nyx-ui');
        Messages.messageReceived.disconnect(onMessageReceived);
    };

});