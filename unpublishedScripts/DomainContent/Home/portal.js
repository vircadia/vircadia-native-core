(function(){
    var teleport;
    var portalDestination;

    function playSound() {
        Audio.playSound(teleport, { volume: 0.40, localOnly: true });
    };

    this.preload = function(entityID) {
        teleport = SoundCache.getSound("atp:/sounds/teleport.raw");

        var properties = Entities.getEntityProperties(entityID);
        portalDestination = properties.userData;

        print("portal.js | The portal destination is " + portalDestination);
    }

    this.enterEntity = function(entityID) {
        print("portal.js | enterEntity");

        var properties = Entities.getEntityProperties(entityID); // in case the userData/portalURL has changed
        portalDestination = properties.userData;

        print("portal.js | enterEntity() .... The portal destination is " + portalDestination);

        if (portalDestination.length > 0) {
            if (portalDestination[0] == '/') {
                print("Teleporting to " + portalDestination);
                Window.location = portalDestination;
            } else if (portalDestination.indexOf('welcome/3.18552,0.164796,-0.300168/0,0.763363,0,0.64597') >= 0) {
                // Backwards compatability: convert existing hardcoded production destination to the current production/development welcome target.
                location.goToEntry();
            } else {
                print("Teleporting to hifi://" + portalDestination);
                Window.location = "hifi://" + portalDestination;
            }
        } else {
            location.goToEntry(); // going forward: no data means go to appropriate entry point
        }

    };

    this.leaveEntity = function(entityID) {
        print("portal.js | leaveEntity");

        playSound();
    };
})
