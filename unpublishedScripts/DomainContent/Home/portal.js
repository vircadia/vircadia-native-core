(function(){
    var teleport;
    var portalDestination;
    var thisEntityID;

    function playSound() {
        var properties = Entities.getEntityProperties(thisEntityID, 'position');
        if (properties) {
            Audio.playSound(teleport, { position: properties.position, volume: 0.40, localOnly: true });
        }
    };

    this.preload = function(entityID) {
        thisEntityID = entityID;
        teleport = SoundCache.getSound("atp:/sounds/teleport.raw");

        var properties = Entities.getEntityProperties(entityID, 'userData');
        if (properties) {
            portalDestination = properties.userData;
            print("portal.js | The portal destination is " + portalDestination);
        }
    }

    this.enterEntity = function(entityID) {
        print("portal.js | enterEntity");

        var properties = Entities.getEntityProperties(entityID, 'userData'); // in case the userData/portalURL has changed
        if (properties) {
            portalDestination = properties.userData;

            print("portal.js | enterEntity() .... The portal destination is " + portalDestination);

            if (portalDestination.length > 0) {
                if (portalDestination[0] == '/') {
                    print("Teleporting to " + portalDestination);
                    Window.location = portalDestination;
                } else {
                    print("Teleporting to hifi://" + portalDestination);
                    Window.location = "hifi://" + portalDestination;
                }
            } else {
                location.goToEntry(); // going forward: no data means go to appropriate entry point
            }
        }
    };

    this.leaveEntity = function(entityID) {
        print("portal.js | leaveEntity");

        playSound();
    };
})
